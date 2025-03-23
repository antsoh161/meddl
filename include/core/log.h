#pragma once
#include <spdlog/spdlog.h>

#include <format>
#include <memory>

namespace meddl::log {

std::shared_ptr<spdlog::logger>& get_logger();

void init(spdlog::level::level_enum level = spdlog::level::info);

//! Use on your own caution, a poorly constructed logger can lead to performance loss
void set_logger(std::shared_ptr<spdlog::logger> logger);

void disable();

template <typename... Args>
void trace(const spdlog::format_string_t<Args...> fmt, Args&&... args)
{
   get_logger()->trace(fmt, std::forward<Args>(args)...);
}

template <typename... Args>
void debug(const spdlog::format_string_t<Args...> fmt, Args&&... args)
{
   get_logger()->debug(fmt, std::forward<Args>(args)...);
}

template <typename... Args>
void info(const spdlog::format_string_t<Args...> fmt, Args&&... args)
{
   get_logger()->info(fmt, std::forward<Args>(args)...);
}

template <typename... Args>
void warn(const spdlog::format_string_t<Args...> fmt, Args&&... args)
{
   get_logger()->warn(fmt, std::forward<Args>(args)...);
}

template <typename... Args>
void error(const spdlog::format_string_t<Args...> fmt, Args&&... args)
{
   get_logger()->error(fmt, std::forward<Args>(args)...);
}

template <typename... Args>
void critical(const spdlog::format_string_t<Args...> fmt, Args&&... args)
{
   get_logger()->critical(fmt, std::forward<Args>(args)...);
}

}  // namespace meddl::log
