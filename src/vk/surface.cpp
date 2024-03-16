#include "vk/surface.h"

#include "GLFW/glfw3.h"

namespace meddl::vk {

Surface::Surface(glfw::Window* window, Instance* instance) : _instance(instance)
{
   auto res = glfwCreateWindowSurface(*_instance, *window, nullptr, &_surface);
   if (res != VK_SUCCESS) {
      throw std::runtime_error{std::format("Failed to create surface, error: {}", static_cast<int32_t>(res))};
   }
}

Surface::~Surface()
{
   if (_surface) {
      // TODO: Allocator
      vkDestroySurfaceKHR(*_instance, _surface, nullptr);
   }
}
}  // namespace meddl::vk
