#pragma once
#include <chrono>
#include <condition_variable>
#include <queue>
#include <thread>
#include <format>

namespace logger {

enum class LogMedia {
  LOG_File,
  LOG_Console,
  LOG_StdOut,
};

enum class LogLevel {
  NONE,
  DEBUG,
  INFO,
  WARN,
  ERROR,
  FATAL,
};


struct LogMessage {
  std::chrono::system_clock::time_point timestamp;
  LogLevel level{LogLevel::NONE};
  std::string message;
};

class AsyncLogger {
 public:
  AsyncLogger();
  ~AsyncLogger();
  AsyncLogger(AsyncLogger&&) = delete;
  AsyncLogger(const AsyncLogger&) = delete;
  AsyncLogger& operator=(AsyncLogger&&) = delete;
  AsyncLogger& operator=(const AsyncLogger&) = delete;

  void start();
  void stop();
  void set_log_media(LogMedia media);

  template <typename... Args>
  void log(const std::format_string<Args...> fmt, Args&&... args) {
    std::lock_guard<std::mutex> lock(m_mutex);
    std::string msg = std::format(fmt, std::forward<Args>(args)...);
    m_shared_queue.push({std::chrono::system_clock::now(), LogLevel::INFO, msg});
    m_cv.notify_one();
  }

 private:
  void log_worker();

  std::optional<LogMedia> m_log_media{LogMedia::LOG_StdOut};
  bool m_enabled{false};
  

  std::jthread m_worker_thread;
  std::queue<LogMessage> m_shared_queue;
  std::mutex m_mutex;
  std::condition_variable m_cv;
};
}  // namespace logger
