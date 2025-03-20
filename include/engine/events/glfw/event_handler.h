#pragma once
#include <utility>

#include "GLFW/glfw3.h"

namespace meddl::events::glfw {

class GlfwEventHandler;
template <typename T>
concept has_registered_user_pointer = requires(T* t) {
   { T::is_user_pointer_registered(std::declval<GLFWwindow*>()) } -> std::same_as<bool>;
};

template <typename T>
concept is_event_registerable =
    std::is_base_of_v<GlfwEventHandler, std::remove_pointer_t<std::decay_t<T>>> ||
    std::is_same_v<GlfwEventHandler, std::remove_pointer_t<std::decay_t<T>>>;

class GlfwEventHandler {
  public:
  private:
};

}  // namespace meddl::events::glfw
