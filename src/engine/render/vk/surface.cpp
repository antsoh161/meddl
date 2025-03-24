#include "engine/render/vk/surface.h"

#include <vulkan/vulkan_core.h>

#include "GLFW/glfw3.h"

namespace meddl::render::vk {
uint64_t Surface::next_id = 1;
Surface::Surface(VkSurfaceKHR surface, Instance* instance)
    : _surface(surface), _instance(instance), _id(next_id++)
{
}

Surface::~Surface()
{
   if (_surface) {
      vkDestroySurfaceKHR(_instance->vk(), _surface, nullptr);
      _surface = VK_NULL_HANDLE;
   }
}

Surface::Surface(Surface&& other) noexcept
    : _surface(other._surface), _instance(other._instance), _id(other._id)
{
   other._surface = VK_NULL_HANDLE;
   other._id = 0;
}

Surface& Surface::operator=(Surface&& other) noexcept
{
   if (this != &other) {
      _surface = other._surface;
      _instance = other._instance;
      _id = other._id;
      other._surface = VK_NULL_HANDLE;
      other._id = 0;
   }
   return *this;
}

template <>
std::expected<Surface, Error> Surface::create<meddl::platform::glfw_window_handle>(
    const meddl::platform::glfw_window_handle& window, Instance* instance)
{
   VkSurfaceKHR surface{nullptr};
   auto res = glfwCreateWindowSurface(instance->vk(), window.native(), nullptr, &surface);
   if (res != VK_SUCCESS) {
      return std::unexpected(Error::from_result(res, "Create surface"));
   }
   return Surface(surface, instance);
}

}  // namespace meddl::render::vk
