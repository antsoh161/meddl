#include "core/engine.h"

#include <vulkan/vulkan_core.h>

#include "vulkan_renderer/vulkan_debug.h"

Engine::~Engine() = default;

void Engine::init_glfw() {
   glfwInit();
   glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
   glfwWindowHint(GLFW_RESIZABLE, GLFW_FALSE);

   // TODO: Builder configuration? Fullscreen etc.
   // WindowBuilder wb;
   // auto w = wb.build();
   // m_windows.push_back(wb.build());
   // m_windows.emplace_back(800, 600, "Meddl Engine");
   m_windows.emplace_back(800, 600, "Meddl");
}

void Engine::init_vulkan() {
}

void Engine::run() {
   while (true) {
      glfwPollEvents();
   }
}
