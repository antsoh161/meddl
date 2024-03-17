#include <fstream>
#include <shaderc/shaderc.hpp>
#include <thread>
#include <print>

#include "wrappers/glfw/window.h"

static std::string readFile(const std::string& filename)
{
   std::ifstream file(filename, std::ios::ate | std::ios::binary);

   if (!file.is_open()) {
      throw std::runtime_error("failed to open file!");
   }

   size_t fileSize = (size_t)file.tellg();
   std::string buffer(fileSize, '\0');  // Create string of appropriate size
   file.seekg(0);
   file.read(buffer.data(), fileSize);
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
   auto window = std::make_shared<meddl::glfw::Window>(WINDOW_WIDTH, WINDOW_HEIGHT, "MeddlExample 1");

   shaderc::Compiler compiler;
   shaderc::CompileOptions options;
   auto vss = readFile("examples/shaders/shader.vert");
   auto fss = readFile("examples/shaders/shader.frag");
   shaderc::SpvCompilationResult vertResult =
       compiler.CompileGlslToSpv(vss.c_str(),
                                 vss.size(),
                                 shaderc_shader_kind::shaderc_vertex_shader,
                                 "vertex_shader.vert",
                                 options);
   if (vertResult.GetCompilationStatus() != shaderc_compilation_status_success) {
      std::println("{}", vertResult.GetErrorMessage());
   }
   else {
      std::println("Success");
   }

   auto counter = 0;
   while (!glfwWindowShouldClose(*window)) {
      glfwPollEvents();
      if (counter >= 50) {
         window->close();
      }
      using namespace std::literals;
      std::this_thread::sleep_for(100ms);
      counter++;
   }
   return EXIT_SUCCESS;
}
