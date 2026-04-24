#pragma once

#include <any>
#include <format>
#include <functional>
#include <memory>
#include <string>
#include <unordered_map>

#include "GLFW/glfw3.h"
#include "core/formatter_utils.h"
#include "core/log.h"
#include "engine/events/event.h"

namespace meddl::glfw {

struct FrameBufferSize {
   uint32_t width;
   uint32_t height;
};

class Monitor;

class Window {
  public:
   Window() noexcept = default;

   Window(GLFWwindow* window) : _handle(window, glfwDestroyWindow) {}

   Window(std::nullptr_t) noexcept : Window{} {}

   Window(uint32_t width,
          uint32_t height,
          const std::string& title,
          const Monitor* monitor = nullptr,
          Window* share = nullptr)
       : _handle(glfwCreateWindow(static_cast<int>(width),
                                  static_cast<int>(height),
                                  title.c_str(),
                                  nullptr,
                                  nullptr),
                 glfwDestroyWindow)
   {
      (void)monitor;
      (void)share;
      glfwSetWindowUserPointer(_handle.get(), this);
   }

   ~Window() = default;

   Window(const Window&) = delete;
   Window& operator=(const Window&) = delete;

   Window(Window&& other) noexcept = default;
   Window& operator=(Window&& other) noexcept = default;

   [[nodiscard]] GLFWwindow* glfw() const { return _handle.get(); }

   bool should_close() { return glfwWindowShouldClose(_handle.get()); }

   void close() { glfwSetWindowShouldClose(_handle.get(), true); }

   [[nodiscard]] FrameBufferSize get_framebuffer_size() const
   {
      int width = 0, height = 0;
      glfwGetFramebufferSize(_handle.get(), &width, &height);
      return {.width = static_cast<uint32_t>(width), .height = static_cast<uint32_t>(height)};
   }

   [[nodiscard]] bool is_minimized() const
   {
      return glfwGetWindowAttrib(_handle.get(), GLFW_ICONIFIED);
   }

   [[nodiscard]] bool is_resized() const { return _is_resized; }
   void reset_resized() { _is_resized = false; }

   void poll_events() { glfwPollEvents(); }

   void register_event_handler(events::EventHandler& handler)
   {
      _event_handler = &handler;
      glfwSetWindowUserPointer(_handle.get(), this);

      glfwSetKeyCallback(
          _handle.get(),
          [](GLFWwindow* window, int key, int scancode, int action, int mods) -> void {
             auto* self = static_cast<Window*>(glfwGetWindowUserPointer(window));
             if (!self || !self->_event_handler) return;

             auto keyCode = static_cast<events::Key>(key);
             auto modifiers = static_cast<events::KeyModifier>(mods);

             if (action == GLFW_PRESS) {
                auto event = events::createEvent<events::KeyPressed>(keyCode, modifiers, false);
                self->_event_handler->dispatch(event);
             }
             else if (action == GLFW_RELEASE) {
                auto event = events::createEvent<events::KeyReleased>(keyCode, modifiers);
                self->_event_handler->dispatch(event);
             }
             else if (action == GLFW_REPEAT) {
                auto event = events::createEvent<events::KeyPressed>(keyCode, modifiers, true);
                self->_event_handler->dispatch(event);
             }
          });
   }

  private:
   bool _is_resized{false};
   events::EventHandler* _event_handler{nullptr};
   std::unique_ptr<GLFWwindow, decltype(&glfwDestroyWindow)> _handle{nullptr, glfwDestroyWindow};
};
}  // namespace meddl::glfw
