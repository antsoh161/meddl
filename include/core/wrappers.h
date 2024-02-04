#pragma once
#include <string>

#include "core/asserts.h"

#include "GLFW/glfw3.h"

namespace meddl {
// class Window {
//  public:
//   Window(uint32_t width,
//          uint32_t height,
//          const std::string &window_name,
//          GLFWmonitor *monitor = nullptr,
//          GLFWwindow *share = nullptr)
//       : m_window(glfwCreateWindow(static_cast<int>(width),
//                                   static_cast<int>(height),
//                                   window_name.c_str(),
//                                   monitor,
//                                   share)) {
//   }
//
//   ~Window() {
//     std::cout << "destroying window...?\n";
//     glfwDestroyWindow(m_window);
//   }
//
//   GLFWwindow *get_underlying() {
//     return m_window;
//   }
//
//  private:
//   GLFWwindow *m_window;
// };
//
// class SurfaceKHR {
//  public:
//   SurfaceKHR(VkInstance instance, Window *window) {
//     M_ASSERT_NOTNULL(window->get_underlying());
//     glfwCreateWindowSurface(instance, window->get_underlying(), nullptr, &m_surface);
//   }
//   ~SurfaceKHR() {
//     vkDestroySurfaceKHR(m_instance, m_surface, nullptr);
//   }
//   SurfaceKHR(SurfaceKHR &&) = delete;
//   SurfaceKHR(const SurfaceKHR &) = delete;
//   SurfaceKHR &operator=(SurfaceKHR &&) = delete;
//   SurfaceKHR &operator=(const SurfaceKHR &) = delete;
//
//  private:
//   VkSurfaceKHR m_surface{};
//   VkInstance m_instance{};
// };
//
// class Monitor2;
//
// class Window2 {
//   Window2(uint32_t width, uint32_t height, const std::string& title, const Monitor2* monitor =
//   nullptr, const Window2* share = nullptr) :
//   Window2(glfwCreateWindow(static_cast<int>(width),static_cast<int>(height), title.c_str(),
//   monitor ? static_cast<GLFWmonitor*>(*monitor) : nullptr, share ?
//   static_cast<GLFWwindow*>(*share) : nullptr)){}
// )
// }
}  // namespace meddl
