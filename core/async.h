#pragma once

#include <exec/static_thread_pool.hpp>
#include <memory>
#include <stdexec/execution.hpp>
#include <string>
#include <unordered_map>

namespace meddl::async {

enum class PoolType { Rendering, Compute, IO, General };
//! @brief Manages thread pools for different workload categories in the engine
class ThreadPoolManager {
  public:
   // Non-copyable
   ThreadPoolManager(const ThreadPoolManager&) = delete;
   ThreadPoolManager& operator=(const ThreadPoolManager&) = delete;

   static ThreadPoolManager& instance();
   void reset(std::optional<uint32_t> max_threads = std::nullopt);
   auto scheduler(PoolType type)
       -> decltype(std::declval<exec::static_thread_pool>().get_scheduler());
   std::shared_ptr<exec::static_thread_pool> create_temporary_pool(const std::string& name,
                                                                   size_t thread_count);

  private:
   ThreadPoolManager() = default;
   ~ThreadPoolManager();

   std::unordered_map<PoolType, std::unique_ptr<exec::static_thread_pool>> _pools;
   std::unordered_map<std::string, std::shared_ptr<exec::static_thread_pool>> _temp_pools;
};
}  // namespace meddl::async
