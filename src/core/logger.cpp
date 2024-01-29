#include "core/logger.h"

#include <iostream>

#include "core/asserts.h"

namespace logger {

AsyncLogger::AsyncLogger() = default;

AsyncLogger::~AsyncLogger() {
   m_enabled = false;
}

void AsyncLogger::start() {
   if (!m_log_media.has_value()) {
      log("No log media selected, defaulting to stdout");
      m_log_media = LogMedia::LOG_StdOut;
   }

   m_enabled = true;
   m_worker_thread = std::jthread(&AsyncLogger::log_worker, this);
}

void AsyncLogger::stop() {
   m_enabled = false;
}

void AsyncLogger::set_log_media(LogMedia media) {
   m_log_media = media;
}

void AsyncLogger::log_worker() {
   while (m_enabled) {
      std::unique_lock<std::mutex> lock(m_mutex);
      m_cv.wait(lock, [this]() {
         return !m_shared_queue.empty() || !m_enabled;
      });

      while (!m_shared_queue.empty()) {
         auto msg = m_shared_queue.front();
         m_shared_queue.pop();

         // Do writes here.. Spawn individual writers...?
         switch (m_log_media.value()) {
            case LOG_StdOut:
               std::cout << "[" << msg.timestamp << "] " << msg.message << "\n";
               break;
            case LOG_File:
               break;
            case LOG_Console:
               break;
            default:
               M_ASSERT_U("Unknown Log media, abort");
         }
      }
   }
}
}  // namespace logger
