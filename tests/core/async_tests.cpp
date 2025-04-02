#include <atomic>
#include <catch2/catch_test_macros.hpp>
#include <catch2/matchers/catch_matchers_exception.hpp>
#include <chrono>
#include <thread>
#include <vector>

#include "core/async.h"
#include "stdexec/__detail/__just.hpp"

// because of catch2
// NOLINTBEGIN (cppcoreguidelines-avoid-do-while)
class AsyncFixture {
  protected:
   meddl::async::ThreadPoolManager& manager = meddl::async::ThreadPoolManager::instance();

   void setup() { manager.reset(); }

   bool run_task_on_scheduler(stdexec::scheduler auto scheduler)
   {
      std::atomic<bool> task_ran{false};

      auto sender = stdexec::starts_on(scheduler, stdexec::just() | stdexec::then([&]() {
                                                     task_ran = true;
                                                     return 42;
                                                  }));

      auto result = stdexec::sync_wait(std::move(sender));
      return task_ran && result && std::get<0>(*result) == 42;
   }
};

TEST_CASE_METHOD(AsyncFixture, "Thread distribution", "[async]")
{
   setup();

   SECTION("All pool types are initialized")
   {
      auto get_render_scheduler = [&]() -> stdexec::scheduler auto {
         return manager.scheduler(meddl::async::PoolType::Rendering);
      };
      auto get_compute_scheduler = [&]() {
         return manager.scheduler(meddl::async::PoolType::Compute);
      };
      auto get_io_scheduler = [&]() { return manager.scheduler(meddl::async::PoolType::IO); };
      auto get_general_scheduler = [&]() {
         return manager.scheduler(meddl::async::PoolType::General);
      };
      REQUIRE_NOTHROW(manager.scheduler(meddl::async::PoolType::Rendering));
      REQUIRE_NOTHROW(manager.scheduler(meddl::async::PoolType::Compute));
      REQUIRE_NOTHROW(manager.scheduler(meddl::async::PoolType::IO));
      REQUIRE_NOTHROW(manager.scheduler(meddl::async::PoolType::General));
   }
}

TEST_CASE_METHOD(AsyncFixture, "Task execution", "[async]")
{
   setup();

   SECTION("Rendering pool executes tasks")
   {
      auto scheduler = manager.scheduler(meddl::async::PoolType::Rendering);
      REQUIRE(run_task_on_scheduler(scheduler));
   }

   SECTION("Compute pool executes tasks")
   {
      auto scheduler = manager.scheduler(meddl::async::PoolType::Compute);
      REQUIRE(run_task_on_scheduler(scheduler));
   }

   SECTION("IO pool executes tasks")
   {
      auto scheduler = manager.scheduler(meddl::async::PoolType::IO);
      REQUIRE(run_task_on_scheduler(scheduler));
   }

   SECTION("General pool executes tasks")
   {
      auto scheduler = manager.scheduler(meddl::async::PoolType::General);
      REQUIRE(run_task_on_scheduler(scheduler));
   }
}

TEST_CASE_METHOD(AsyncFixture, "Temporary pools", "[async]")
{
   setup();

   SECTION("Creating a temporary pool")
   {
      auto pool1 = manager.create_temporary_pool("test_pool", 2);
      REQUIRE(pool1 != nullptr);

      auto pool2 = manager.create_temporary_pool("test_pool", 4);
      REQUIRE(pool1 == pool2);

      auto pool3 = manager.create_temporary_pool("other_pool", 2);
      REQUIRE(pool1 != pool3);
   }

   SECTION("Temporary pool executes tasks")
   {
      auto pool = manager.create_temporary_pool("test_exec_pool", 2);
      auto scheduler = pool->get_scheduler();
      REQUIRE(run_task_on_scheduler(scheduler));
   }
}

TEST_CASE_METHOD(AsyncFixture, "Multiple tasks execute concurrently", "[async]")
{
   setup();

   SECTION("Tasks run in parallel on the compute pool")
   {
      auto scheduler = manager.scheduler(meddl::async::PoolType::Compute);

      constexpr int num_tasks = 10;
      std::atomic<int> counter{0};
      std::atomic<int> max_concurrent{0};
      std::atomic<int> currently_running{0};

      auto task = [&](size_t i) {
         currently_running++;
         int current = currently_running.load();
         int previous_max = max_concurrent.load();
         while (current > previous_max &&
                !max_concurrent.compare_exchange_weak(previous_max, current)) {
            current = currently_running.load();
            previous_max = max_concurrent.load();
         }

         std::this_thread::sleep_for(std::chrono::milliseconds(50));

         currently_running--;
         counter++;
         return i;
      };

      // TODO: There has to be a better way?
      auto senders =
          std::views::iota(0, num_tasks) | std::views::transform([&](int i) {
             return stdexec::starts_on(scheduler, stdexec::just(i) | stdexec::then(task));
          });
      auto sender_tuple = [&]<std::size_t... Is>(std::index_sequence<Is...>) {
         return std::make_tuple(std::move(senders[Is])...);
      }(std::make_index_sequence<num_tasks>{});
      auto all_tasks = std::apply(stdexec::when_all, std::move(sender_tuple));

      // auto all =
      //     stdexec::when_all(stdexec::starts_on(scheduler, stdexec::just(0) |
      //     stdexec::then(task)),
      //                       stdexec::starts_on(scheduler, stdexec::just(1) |
      //                       stdexec::then(task)));
      stdexec::sync_wait(std::move(all_tasks));

      REQUIRE(counter == num_tasks);
      REQUIRE(max_concurrent >= 2);
   }
}
// NOLINTEND (cppcoreguidelines-avoid-do-while)
