#pragma once

#include <expected>
#include <memory>
#include <string>

#include "engine/platform/window_handle.h"

namespace meddl::render {

// Forward declarations for render API types
namespace vk_later {
class Instance;
struct surface_handle;
}  // namespace vk

namespace wgpu {
class Context;
struct surface_handle;
}  // namespace wgpu

struct surface_error {
   std::string message;
   int code;
};

struct vulkan_api {};
struct wgpu_api {};

struct surface_id {
   uint64_t id{0};

   [[nodiscard]] bool valid() const { return id != 0; }

   bool operator==(const surface_id&) const = default;
};

template <typename API, typename WindowHandle>
concept compatible_api_window =
    (std::is_same_v<API, vulkan_api> && std::is_same_v<WindowHandle, platform::glfw_window_handle>)
#ifdef EMSCRIPTEN
    || (std::is_same_v<API, webgpu_api> &&
        std::is_same_v<WindowHandle, platform::webgpu_canvas_handle>)
#endif
    ;

template <typename API>
class Surface {
  public:
   Surface(const Surface&) = delete;
   Surface(Surface&&) = delete;
   Surface& operator=(const Surface&) = delete;
   Surface& operator=(Surface&&) = delete;
   virtual ~Surface() = default;

   [[nodiscard]] virtual surface_id id() const = 0;

   [[nodiscard]] virtual void* native_handle() const = 0;

   template <platform::window_handle WindowHandle>
      requires compatible_api_window<API, WindowHandle>
   static std::expected<std::unique_ptr<Surface<API>>, surface_error> create(
       const WindowHandle& window, void* api_instance);
};

}  // namespace meddl::renderer
