#include "core/asserts.h"
#include "vulkan_renderer/context.h"
#include "wrappers/glfw/window.h"

int main() {
   // Engine engine;
   constexpr uint32_t WINDOW_WIDTH = 800;
   constexpr uint32_t WINDOW_HEIGHT = 600;

   glfwInit();
   glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
   glfwWindowHint(GLFW_RESIZABLE, GLFW_FALSE);

   auto window = std::make_shared<meddl::glfw::Window>(WINDOW_WIDTH, WINDOW_HEIGHT, "Meddl Test");

   try {
      auto ctx = meddl::vulkan::ContextBuilder()
                     .window(window)
                     .enable_debugger()
                     .with_debug_layers({"VK_LAYER_KHRONOS_validation"})
                     .build();
    while(true)
      ;
   } catch (const std::exception& e) {
      M_ASSERT_U("Initialization caught exception: {}", e.what())
   }
   // engine.run();
   return EXIT_SUCCESS;
}
