#pragma once

#include <iostream>
#include <memory>
#include <string>

#include "GLFW/glfw3.h"

namespace meddl::glfw {

class Monitor;

class Window {
  public:
   Window() noexcept = default;

   Window(GLFWwindow* window) : window_handle(window, glfwDestroyWindow)
   {
   }

   Window(std::nullptr_t) noexcept : Window{}
   {
   }

   Window(uint32_t width,
          uint32_t height,
          const std::string& title,
          const Monitor* monitor = nullptr,
          Window* share = nullptr)
       : window_handle(glfwCreateWindow(static_cast<int>(width),
                                        static_cast<int>(height),
                                        title.c_str(),
                                        nullptr,
                                        nullptr),
                       glfwDestroyWindow)
   {
   }

   ~Window() = default;  //=default;

   Window(const Window&) = delete;

   Window& operator=(const Window&) = delete;

   Window(Window&& other) noexcept = default;

   Window& operator=(Window&& other) noexcept = default;

   operator GLFWwindow*() const
   {
      return window_handle.get();
   }

   void should_close(bool value)
   {
      glfwWindowShouldClose(window_handle.get());
   }

   void close()
   {
      glfwSetWindowShouldClose(window_handle.get(), true);
   }

  private:
   std::unique_ptr<GLFWwindow, decltype(&glfwDestroyWindow)> window_handle{nullptr,
                                                                           glfwDestroyWindow};
};

}  // namespace meddl::glfw
