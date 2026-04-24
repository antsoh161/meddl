#pragma once

#include <concepts>

#include "GLFW/glfw3.h"

namespace meddl::platform {
struct glfw_window_handle {
   using native_handle_type = GLFWwindow*;
   GLFWwindow* handle;

   explicit operator GLFWwindow*() const { return handle; }
   [[nodiscard]] GLFWwindow* native() const { return handle; }
};

#ifdef EMSCRIPTEN
struct webgpu_canvas_handle {
   using native_handle_type = const char*;

   const char* canvas_id;

   explicit operator const char*() const { return canvas_id; }
};
#endif
template <typename T>
concept window_handle = requires(const T& t) {
   {
      static_cast<typename T::native_handle_type>(t)
   } -> std::same_as<typename T::native_handle_type>;
};

// Verify our implementations satisfy the concept
// static_assert(window_handle<glfw_window_handle>,
//               "glfw_window_handle must satisfy window_handle concept");

}  // namespace meddl::platform
