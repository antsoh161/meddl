#pragma once

#include <vulkan/vulkan_core.h>

#include "engine/platform/window_handle.h"
#include "engine/render/surface.h"
#include "engine/render/vk/instance.h"

struct VkSurfaceKHR_T;
using VkSurfaceKHR = VkSurfaceKHR_T*;

namespace meddl::render::vk {

class Instance;
class Surface {
  public:
   template <platform::window_handle WindowHandle>
   static std::expected<Surface, surface_error> create(const WindowHandle& window,
                                                       Instance* instance);

   ~Surface();

   Surface(const Surface&) = delete;
   Surface& operator=(const Surface&) = delete;

   Surface(Surface&&) noexcept;
   Surface& operator=(Surface&&) noexcept;

   [[nodiscard]] VkSurfaceKHR vk() const { return _surface; }

  private:
   Surface(VkSurfaceKHR surface, Instance* instance);

   VkSurfaceKHR _surface{VK_NULL_HANDLE};
   Instance* _instance;
   uint64_t _id{0};

   static uint64_t next_id;  // hmm?
};

template <>
std::expected<Surface, surface_error> Surface::create<meddl::platform::glfw_window_handle>(
    const meddl::platform::glfw_window_handle& window, meddl::render::vk::Instance* instance);

}  // namespace meddl::render::vk
