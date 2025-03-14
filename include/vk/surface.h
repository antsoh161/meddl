#pragma once

#include "GLFW/glfw3.h"
#include "vk/instance.h"
#include "engine/window.h"

namespace meddl::vk {
class Surface {
  public:
   Surface(glfw::Window* window, Instance* instance);
   ~Surface();

   Surface(const Surface&) = delete;
   Surface& operator=(const Surface&) = delete;

   Surface(Surface&&) = default;
   Surface& operator=(Surface&&) = default;

   operator VkSurfaceKHR() const { return _surface; }
   [[nodiscard]] VkSurfaceKHR vk() const { return _surface; }

  private:
   VkSurfaceKHR _surface{VK_NULL_HANDLE};
   Instance* _instance;
};

}  // namespace meddl::vk
