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

   void init_glfw();
   void init_vulkan();
   void run();

  private:
   std::vector<meddl::glfw::Window> m_windows;

   logger::AsyncLogger m_logger{};

};
