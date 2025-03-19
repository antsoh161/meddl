#pragma once

#include <expected>
#include <string>

namespace meddl::platform {
struct platform_error {
   std::string message;
   int code;
};

namespace glfw {
class Window;
}
namespace webgpu {
class Canvas;
}

template <typename T>
concept window_provider = requires(T t, int32_t width, int32_t height, std::string_view title) {
   {
      t.create_window(width, height, title)
   } -> std::same_as<std::expected<typename T::window_type, platform_error>>;
   { t.destroy_window(std::declval<typename T::window_type>()) } -> std::same_as<void>;
   { t.poll_events() } -> std::same_as<bool>;
};

struct desktop_platform {};
struct web_platform {};

}  // namespace meddl::platform
