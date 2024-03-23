#include <print>

#include "gtest/gtest.h"
#include "vk/debug.h"
#include "vk/defaults.h"
#include "vk/device.h"
#include "vk/instance.h"
#include "vk/shader.h"
#include "vk/surface.h"
#include "vk/swapchain.h"

using namespace meddl::vk;
using namespace meddl::glfw;

class MeddlFixture : public ::testing::Test {
  public:
   void init_instance()
   {
      glfwInit();
      glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
      glfwWindowHint(GLFW_RESIZABLE, GLFW_FALSE);
      auto app_info = meddl::vk::defaults::app_info();
      auto debug_info = meddl::vk::defaults::debug_info();
      _instance = std::make_unique<Instance>(app_info, debug_info, _debugger);
   }
   void init_window() { _window = std::make_shared<Window>(800, 600, "Test Window"); }
   void init_surface() { _surface = std::make_unique<Surface>(_window.get(), _instance.get()); }
   void init_device()
   {
      std::optional<int> present_index{};
      std::optional<int> graphics_index{};

      for (const auto& device : _instance->get_physical_devices()) {
         present_index = device->get_present_family(_surface.get());
         graphics_index = device->get_queue_family(VK_QUEUE_GRAPHICS_BIT);
      }

      auto physical_device = _instance->get_physical_devices().front();
      auto extensions = meddl::vk::defaults::device_extensions();

      auto config = QueueConfiguration(present_index.value());
      std::unordered_map<uint32_t, QueueConfiguration> configs = {{graphics_index.value(), config}};
      _device = std::make_unique<Device>(
          physical_device.get(), configs, extensions, std::nullopt, _debugger);
   }
   void init_swapchain()
   {
      SwapchainOptions options{};
      _swapchain = std::make_unique<Swapchain>(_instance->get_physical_devices().front().get(),
                                               _device.get(),
                                               _surface.get(),
                                               options,
                                               _window->get_framebuffer_size());
   }
   void init_all()
   {
      init_instance();
      init_window();
      init_surface();
      init_device();
      init_swapchain();
   }
   std::optional<Debugger> _debugger{};
   std::unique_ptr<Instance> _instance{};
   std::shared_ptr<Window> _window{};
   std::unique_ptr<Surface> _surface{};
   std::unique_ptr<Device> _device{};
   std::unique_ptr<Swapchain> _swapchain{};
};

TEST_F(MeddlFixture, Initialization)
{
   EXPECT_NO_THROW(init_all());
}

TEST_F(MeddlFixture, CompileShaders)
{
   init_all();
   ShaderCompiler compiler;
   auto vertex_spirv =
       compiler.compile(std::filesystem::current_path() / "shader.vert", shaderc_vertex_shader);
   EXPECT_EQ(vertex_spirv.size(), 376);
   std::unique_ptr<ShaderModule> vert_mod;
   EXPECT_NO_THROW(std::make_unique<ShaderModule>(_device.get(), vertex_spirv));

   auto frag_spirv = 
       compiler.compile(std::filesystem::current_path() / "shader.frag ", shaderc_fragment_shader);
   EXPECT_EQ(frag_spirv.size(), 143);
   std::unique_ptr<ShaderModule> frag_mod;
   EXPECT_NO_THROW(std::make_unique<ShaderModule>(_device.get(), frag_spirv));
}

//! @brief
//! We require atleast the graphics bit, and present mode to be able to draw.
// TEST_F(MeddlFixture, BareMinimumRequirements)
// {
//    init();
//    EXPECT_GT(_instance->get_physical_devices().size(), 0);
// }

// TEST_F(MeddlFixture, TestDevice)
// {
//    init();
//    auto physical_device = _instance->get_physical_devices().front();
//    auto extensions = meddl::vk::defaults::device_extensions();
//
//    std::optional<int> present_index = physical_device->get_queue_family(VK_QUEUE_GRAPHICS_BIT);
//    std::optional<int> graphics_index = physical_device->get_present_family(_surface.get());
//    EXPECT_EQ(graphics_index.value(), present_index.value());
//    auto config = QueueConfiguration(present_index.value());
//    std::unordered_map<uint32_t, QueueConfiguration> configs = {{graphics_index.value(), config}};
//    auto device = std::make_unique<NewDevice>(
//        physical_device.get(), configs, extensions, std::nullopt, _debugger);
// }
