#include <print>
#include <shaderc/shaderc.hpp>
#include <thread>

#include "GLFW/glfw3.h"
#include "engine/renderer.h"
#include "engine/vertex.h"

auto main() -> int
{
   meddl::Renderer renderer;
   constexpr auto framerate = 16;
   while (true) {
      glfwPollEvents();
      renderer.draw();
      std::this_thread::sleep_for(std::chrono::milliseconds(framerate));
   }

   return EXIT_SUCCESS;
}
