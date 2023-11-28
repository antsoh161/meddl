#include <thread>
#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

#include "core/asserts.h"
#include "core/window.h"


int main() {
  WindowConfig cfg;
  std::cout << "Default config\n width = " << cfg.width << "\n height = " << cfg.height << "\n";
  WindowBuilder wb(cfg);
  auto window = wb.build();
  std::this_thread::sleep_for(std::chrono::seconds(10));
  return EXIT_SUCCESS;
}
