#include <fstream>
#include <memory>
#include <print>
#include <shaderc/shaderc.hpp>
#include <thread>

#include "app/window.h"
#include "vk/shader.h"

static std::string readFile(const std::string& filename)
{
   std::ifstream file(filename, std::ios::ate | std::ios::binary);

   if (!file.is_open()) {
      throw std::runtime_error("failed to open file!");
   }

   size_t fileSize = (size_t)file.tellg();
   std::string buffer(fileSize, '\0');  // Create string of appropriate size
   file.seekg(0);
   file.read(buffer.data(), static_cast<std::streamsize>(fileSize));
   file.close();
   return buffer;
}

auto main() -> int
{
   constexpr uint32_t WINDOW_WIDTH = 800;
   constexpr uint32_t WINDOW_HEIGHT = 600;

   glfwInit();
   glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
   glfwWindowHint(GLFW_RESIZABLE, GLFW_FALSE);
   auto window =
       std::make_shared<meddl::glfw::Window>(WINDOW_WIDTH, WINDOW_HEIGHT, "MeddlExample 1");

   meddl::vk::ShaderCompiler compiler;
   auto vertex_code = compiler.compile("/home/anton/workspace/meddl/examples/shaders/shader.vert",
                                       shaderc_shader_kind::shaderc_vertex_shader);
   auto fragment_code = compiler.compile("/home/anton/workspace/meddl/examples/shaders/shader.frag",
                                         shaderc_shader_kind::shaderc_fragment_shader);

   std::println("vert size: {}", vertex_code.size());
   std::println("frag size: {}", fragment_code.size());

   auto counter = 0;
   const auto maxCounter = 50;
   while (!glfwWindowShouldClose(*window)) {
      glfwPollEvents();
      if (counter >= maxCounter) {
         window->close();
      }
      using namespace std::literals;
      std::this_thread::sleep_for(100ms);
      counter++;
   }
   return EXIT_SUCCESS;
}
