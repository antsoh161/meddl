#pragma once

#include <memory>
#include <string>

#include "GLFW/glfw3.h"
#include "core/formatter_utils.h"

namespace meddl::glfw {
enum class WindowEvent {
   Resize,
   Close,
   Focus,
   Iconify,
   KeyPress,
   MouseButton,
   MouseMove,
   // ...
};

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
      (void)monitor;
      (void)share;
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

template <>
struct std::formatter<meddl::glfw::WindowEvent> {
   enum class FormatStyle { Normal, Short, Debug };
   FormatStyle style = FormatStyle::Normal;

   constexpr auto parse(std::format_parse_context& ctx)
   {
      auto it = ctx.begin();
      if (it != ctx.end() && *it != '}') {
         if (*it == 's') {
            style = FormatStyle::Short;
            ++it;
         }
         else if (*it == 'd') {
            style = FormatStyle::Debug;
            ++it;
         }
      }
      return it;
   }

   auto format(const meddl::glfw::WindowEvent& event, std::format_context& ctx) const
   {
      constexpr auto to_string = [](meddl::glfw::WindowEvent e) constexpr -> std::string_view {
         switch (e) {
            case meddl::glfw::WindowEvent::Resize:
               return "Resize";
            case meddl::glfw::WindowEvent::Close:
               return "Close";
            case meddl::glfw::WindowEvent::Focus:
               return "Focus";
            case meddl::glfw::WindowEvent::Iconify:
               return "Iconify";
            case meddl::glfw::WindowEvent::KeyPress:
               return "KeyPress";
            case meddl::glfw::WindowEvent::MouseButton:
               return "MouseButton";
            case meddl::glfw::WindowEvent::MouseMove:
               return "MouseMove";
            default:
               return "Unknown";
         }
      };

      constexpr auto to_short_string =
          [](meddl::glfw::WindowEvent e) constexpr -> std::string_view {
         switch (e) {
            case meddl::glfw::WindowEvent::Resize:
               return "RSZ";
            case meddl::glfw::WindowEvent::Close:
               return "CLO";
            case meddl::glfw::WindowEvent::Focus:
               return "FOC";
            case meddl::glfw::WindowEvent::Iconify:
               return "ICO";
            case meddl::glfw::WindowEvent::KeyPress:
               return "KEY";
            case meddl::glfw::WindowEvent::MouseButton:
               return "BTN";
            case meddl::glfw::WindowEvent::MouseMove:
               return "MOV";
            default:
               return "UNK";
         }
      };

      switch (style) {
         case FormatStyle::Short:
            return std::format_to(ctx.out(), "{}", to_short_string(event));
         case FormatStyle::Debug:
            return std::format_to(ctx.out(), "{}({})", to_string(event), static_cast<int>(event));
         case FormatStyle::Normal:
         default:
            return std::format_to(ctx.out(), "{}", to_string(event));
      }
   }
};
