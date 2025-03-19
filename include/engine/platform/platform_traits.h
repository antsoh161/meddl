#pragma once

#include "GLFW/glfw3.h"
namespace meddl::platform {
struct Glfw;
struct WebGPU;

template <typename T>
struct PlatformTraits {
   static_assert(sizeof(T) == 0, "Unimplemented platform type");
};

template <>
struct PlatformTraits<Glfw> {
   using Window = struct GLFWwindow*;
   using Monitor = struct GLFWmonitor*;

   using KeyCode = int;
   using ScanCode = int;
   using ModCode = int;
   using MouseButton = int;
};
}  // namespace meddl::platform
