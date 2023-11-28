#include <cstdint>
#include <memory>

#include "GLFW/glfw3.h"

struct DestroyglfwWin{

    void operator()(GLFWwindow* ptr){
         glfwDestroyWindow(ptr);
    }

};

class TOMLFile;

struct WindowConfig {
  WindowConfig();
  void load(TOMLFile& file);

  uint32_t width;
  uint32_t height;
  bool fullscreen;
};

class WindowBuilder {
 private:
  WindowConfig config;

 public:
  WindowBuilder() = default;
  WindowBuilder(const WindowConfig cfg);

  WindowBuilder& width(uint32_t width);
  WindowBuilder& height(uint32_t height);
  WindowBuilder& fullscreen(bool fullscreen);

  std::unique_ptr<GLFWwindow, DestroyglfwWin> build();
};
