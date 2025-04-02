#include "core/async.h"

#include <thread>

#include "core/log.h"

namespace meddl::async {

ThreadPoolManager& ThreadPoolManager::instance()
{
   static ThreadPoolManager instance;
   return instance;
}

void ThreadPoolManager::reset(std::optional<uint32_t> max_threads)
{
   uint32_t no_threads =
       max_threads.has_value() ? max_threads.value() : std::thread::hardware_concurrency();
   //! TODO: Is there a set of right numbers?
   uint32_t available_threads = std::max(1u, no_threads - 2);
   uint32_t render_threads = std::max(1u, available_threads / 3);
   uint32_t compute_threads = std::max(1u, available_threads / 3);
   uint32_t io_threads = std::max(2u, available_threads / 6);
   uint32_t general_threads =
       std::max(1u, available_threads - (render_threads + compute_threads + io_threads));

   meddl::log::debug("Thread pool initialized with:");
   _pools[PoolType::Rendering] = std::make_unique<exec::static_thread_pool>(render_threads);
   meddl::log::debug("Render: {}", render_threads);
   _pools[PoolType::Compute] = std::make_unique<exec::static_thread_pool>(compute_threads);
   meddl::log::debug("Compute: {}", compute_threads);
   _pools[PoolType::IO] = std::make_unique<exec::static_thread_pool>(io_threads);
   meddl::log::debug("IO: {}", io_threads);
   _pools[PoolType::General] = std::make_unique<exec::static_thread_pool>(general_threads);
   meddl::log::debug("General: {}", general_threads);
}

auto ThreadPoolManager::scheduler(PoolType type)
    -> decltype(std::declval<exec::static_thread_pool>().get_scheduler())
{
   auto it = _pools.find(type);
   if (it == _pools.end()) {
      it = _pools.find(PoolType::General);
      if (it == _pools.end()) {
         reset();
         it = _pools.find(PoolType::General);
      }
   }
   return it->second->get_scheduler();
}

std::shared_ptr<exec::static_thread_pool> ThreadPoolManager::create_temporary_pool(
    const std::string& name, size_t thread_count)
{
   auto it = _temp_pools.find(name);
   if (it != _temp_pools.end()) {
      return it->second;
   }

   auto pool = std::make_shared<exec::static_thread_pool>(thread_count);
   _temp_pools[name] = pool;
   return pool;
}

ThreadPoolManager::~ThreadPoolManager()
{
   _temp_pools.clear();
   _pools.clear();
}

}  // namespace meddl::async
