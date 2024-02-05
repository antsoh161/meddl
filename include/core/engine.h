#pragma once

#include <vector>

#include "core/logger.h"
#include "wrappers/glfw/window.h"

class Engine {
  public:
   Engine() = default;
   ~Engine();
   Engine(Engine &&) = delete;
   Engine(const Engine &) = delete;
   Engine &operator=(Engine &&) = delete;
   Engine &operator=(const Engine &) = delete;

  private:
   logger::AsyncLogger m_logger{};
};
