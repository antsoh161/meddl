#include "core/log.h"

#include <mutex>

#include "spdlog/async.h"
#include "spdlog/sinks/stdout_color_sinks.h"

namespace {
constexpr auto THREAD_POOL_SIZE = 8192;
constexpr auto THREAD_COUNT = 1;
}  // namespace
namespace meddl::log {
std::shared_ptr<spdlog::logger>& get_logger()
{
   static std::mutex s_logger_mutex;
   static std::shared_ptr<spdlog::logger> s_logger;
   static std::once_flag s_init_flag;

   std::call_once(s_init_flag, []() {
      spdlog::init_thread_pool(THREAD_POOL_SIZE, THREAD_COUNT);
      auto console_sink = std::make_shared<spdlog::sinks::stdout_color_sink_mt>();
      s_logger = std::make_shared<spdlog::async_logger>(
          "meddl", console_sink, spdlog::thread_pool(), spdlog::async_overflow_policy::block);
      s_logger->set_pattern("%^[%H:%M:%S.%e] [%l] [%t] %v%$");
      s_logger->set_level(spdlog::level::info);
      spdlog::register_logger(s_logger);
   });

   return s_logger;
}

void init(spdlog::level::level_enum level)
{
   auto& logger = get_logger();
   logger->set_level(level);
}

void set_logger(std::shared_ptr<spdlog::logger> logger)
{
   static std::mutex s_logger_mutex;
   std::lock_guard<std::mutex> lock(s_logger_mutex);
   get_logger() = logger;
}

void disable()
{
   get_logger()->set_level(spdlog::level::off);
}

}  // namespace meddl::log
