#pragma once

#include <expected>
#include <string>

namespace meddl::platform {
struct platform_error {
   std::string message;
   int code;
};

struct glfw_window_handle {
   void* glfw;
};
struct win32_window_handle {
   void* hwnd;
   void* hinstance;
};
struct x11_window_handle {
   void* display;
   unsigned long window;
};
struct wayland_window_handle {
   void* display;
   void* surface;
};
struct metal_window_handle {
   void* nsview;
};
struct android_window_handle {
   void* window;
};
struct web_canvas_handle {
   const char* canvas_id;
};

}  // namespace meddl::platform
