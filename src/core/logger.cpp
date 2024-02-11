#include "core/logger.h"

#include <iostream>
#include "core/asserts.h"

namespace logger {

AsyncLogger::AsyncLogger() = default;

AsyncLogger::~AsyncLogger() {
   _enabled = false;
}

void AsyncLogger::start() {
   if (!_log_media.has_value()) {
      log("No log media selected, defaulting to stdout");
      _log_media = LogMedia::LOG_StdOut;
   }

   _enabled = true;
   _worker_thread = std::jthread(&AsyncLogger::log_worker, this);
}

void AsyncLogger::stop(){
   _enabled = false;
}

void AsyncLogger::set_log_media(LogMedia media) {
   _log_media = media;
}

void AsyncLogger::log_worker() {
   while (_enabled) {
      std::unique_lock<std::mutex> lock(m_mutex);
      m_cv.wait(lock, [this]() {
         return !_shared_queue.empty() || !_enabled;
      });

      while (!_shared_queue.empty()) {
         auto msg = _shared_queue.front();
         _shared_queue.pop();

         // Do writes here.. Spawn individual writers...?
         switch (_log_media.value()) {
            case LogMedia::LOG_File:
               break;
            case LogMedia::LOG_Console:
               break;
            case LogMedia::LOG_StdOut:
               std::cout << "[" << msg.timestamp << "] " << msg.message << "\n";
               break;
         }
      }
   }
}
}  // namespace logger
