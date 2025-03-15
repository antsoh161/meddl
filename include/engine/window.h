#pragma once

#include <memory>
#include <string>

#include "GLFW/glfw3.h"

namespace meddl::glfw {

struct FrameBufferSize {
   uint32_t width;
   uint32_t height;
};

class Monitor;

class Window {
  public:
   Window() noexcept = default;

   Window(GLFWwindow* window) : _window_handle(window, glfwDestroyWindow) {}

   Window(std::nullptr_t) noexcept : Window{} {}

   Window(uint32_t width,
          uint32_t height,
          const std::string& title,
          const Monitor* monitor = nullptr,
          Window* share = nullptr)
       : _window_handle(glfwCreateWindow(static_cast<int>(width),
                                         static_cast<int>(height),
                                         title.c_str(),
                                         nullptr,
                                         nullptr),
                        glfwDestroyWindow)
   {
      glfwSetWindowUserPointer(_window_handle.get(), this);
      glfwSetFramebufferSizeCallback(_window_handle.get(), [](GLFWwindow* window, int, int) {
         auto* self = static_cast<Window*>(glfwGetWindowUserPointer(window));
         self->_is_resized = true;
      });
   }

   ~Window() = default;

   Window(const Window&) = delete;
   Window& operator=(const Window&) = delete;

   Window(Window&& other) noexcept = default;
   Window& operator=(Window&& other) noexcept = default;

   // operator GLFWwindow*() const { return window_handle.get(); }
   [[nodiscard]] GLFWwindow* glfw() const { return _window_handle.get(); }

   bool should_close() { return glfwWindowShouldClose(_window_handle.get()); }

   void close() { glfwSetWindowShouldClose(_window_handle.get(), true); }

   [[nodiscard]] FrameBufferSize get_framebuffer_size() const
   {
      int width = 0, height = 0;
      glfwGetFramebufferSize(_window_handle.get(), &width, &height);
      return {.width = static_cast<uint32_t>(width), .height = static_cast<uint32_t>(height)};
   }

   [[nodiscard]] bool is_minimized() const
   {
      return glfwGetWindowAttrib(_window_handle.get(), GLFW_ICONIFIED);
   }

   [[nodiscard]] bool is_resized() const { return _is_resized; }
   void reset_resized() { _is_resized = false; }

  private:
   bool _is_resized{false};
   std::unique_ptr<GLFWwindow, decltype(&glfwDestroyWindow)> _window_handle{nullptr,
                                                                            glfwDestroyWindow};
};

}  // namespace meddl::glfw
