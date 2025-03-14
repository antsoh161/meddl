#include <print>
#include <shaderc/shaderc.hpp>
#include <thread>

#include "GLFW/glfw3.h"
#include "engine/renderer.h"


auto main() -> int
{

   meddl::Renderer renderer;
   while (true) {
      glfwPollEvents();
      renderer.draw();
      std::this_thread::sleep_for(std::chrono::milliseconds(16));
   }

   return EXIT_SUCCESS;
}
