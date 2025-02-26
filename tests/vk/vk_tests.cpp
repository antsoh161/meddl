#include <catch2/catch_test_macros.hpp>
#include <print>

#include "vk/command.h"
#include "vk/debug.h"
#include "vk/defaults.h"
#include "vk/device.h"
#include "vk/instance.h"
#include "vk/pipeline.h"
#include "vk/shader.h"
#include "vk/surface.h"
#include "vk/swapchain.h"

using namespace meddl::vk;
using namespace meddl::glfw;
// NOLINTBEGIN (cppcoreguidelines-avoid-do-while)

class MeddlFixture {
  public:
   void init_instance()
   {
      glfwInit();
      glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
      glfwWindowHint(GLFW_RESIZABLE, GLFW_FALSE);
      glfwWindowHint(GLFW_VISIBLE, GLFW_FALSE);  // Headless for tests
      auto app_info = meddl::vk::defaults::app_info();
      auto debug_info = meddl::vk::defaults::debug_info();
      _instance = std::make_unique<Instance>(app_info, debug_info, _debugger);
   }
   void init_debugger()
   {
      _debugger = std::make_optional<Debugger>();
      _debugger->add_validation_layer("VK_LAYER_KHRONOS_validation");
   }
   void init_window()
   {
      _window = std::make_shared<Window>(
          defaults::DEFAULT_WINDOW_HEIGHT, defaults::DEFAULT_WINDOW_WIDTH, "Test Window");
   }
   void init_surface() { _surface = std::make_unique<Surface>(_window.get(), _instance.get()); }
   void init_device()
   {
      std::optional<int> present_index{};
      std::optional<int> graphics_index{};

      for (const auto& device : _instance->get_physical_devices()) {
         present_index = device->get_present_family(_surface.get());
         graphics_index = device->get_queue_family(VK_QUEUE_GRAPHICS_BIT);
         std::println("Present: {}, Graphics: {}", present_index.value(), graphics_index.value());
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
                                               _renderpass.get(),
                                               options,
                                               _window->get_framebuffer_size());
   }

   void init_renderpass()
   {
      auto color_attachement = defaults::color_attachment(defaults::DEFAULT_IMAGE_FORMAT);
      _renderpass = std::make_unique<RenderPass>(_device.get(), color_attachement);
   }

   void init_pipeline()
   {
      ShaderCompiler compiler;
      auto vertex_spirv =
          compiler.compile(std::filesystem::current_path() / "shader.vert", shaderc_vertex_shader);
      auto vertex_module = ShaderModule(_device.get(), vertex_spirv);

      auto frag_spirv = compiler.compile(std::filesystem::current_path() / "shader.frag",
                                         shaderc_fragment_shader);
      auto frag_module = ShaderModule(_device.get(), frag_spirv);

      _pipeline_layout = std::make_unique<PipelineLayout>(_device.get(), 0);
      _graphics_pipeline = std::make_unique<GraphicsPipeline>(
          vertex_module, frag_module, _device.get(), _pipeline_layout.get(), _renderpass.get());
   };

   void init_command()
   {
      // TODO: 0 is not always the correct index, save somewhere to fetch for the commandpool
      _command_pool = std::make_unique<CommandPool>(
          _device.get(), 0, meddl::vk::defaults::DEFAULT_COMMAND_POOL_FLAGS);
      _command_buffer = std::make_unique<CommandBuffer>(_device.get(), _command_pool.get());
   }
   void init_all()
   {
      init_debugger();
      init_instance();
      init_window();
      init_surface();
      init_device();
      init_renderpass();
      init_swapchain();
      init_pipeline();
      init_command();
   }
   std::optional<Debugger> _debugger{};
   std::unique_ptr<Instance> _instance{};
   std::shared_ptr<Window> _window{};
   std::unique_ptr<Surface> _surface{};
   std::unique_ptr<Device> _device{};
   std::unique_ptr<Swapchain> _swapchain{};
   std::unique_ptr<PipelineLayout> _pipeline_layout{};
   std::unique_ptr<RenderPass> _renderpass{};
   std::unique_ptr<GraphicsPipeline> _graphics_pipeline{};
   std::unique_ptr<CommandPool> _command_pool{};
   std::unique_ptr<CommandBuffer> _command_buffer{};
};

TEST_CASE_METHOD(MeddlFixture, "Initialization")
{
   REQUIRE_NOTHROW(init_all());  // NOLINT
}

TEST_CASE_METHOD(MeddlFixture, "CommandBufferLifecycle")
{
   init_all();
   REQUIRE(_command_buffer->state() == CommandBuffer::State::Ready);
   REQUIRE_NOTHROW(_command_buffer->begin());
   REQUIRE(_command_buffer->state() == CommandBuffer::State::Recording);
   REQUIRE_NOTHROW(_command_buffer->end());
   REQUIRE(_command_buffer->state() == CommandBuffer::State::Executable);
   REQUIRE_NOTHROW(_command_buffer->reset());
   REQUIRE(_command_buffer->state() == CommandBuffer::State::Ready);

   REQUIRE_NOTHROW(_command_buffer->begin(VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT));
   REQUIRE_NOTHROW(_command_buffer->end());
   REQUIRE_NOTHROW(_command_buffer->reset(VK_COMMAND_BUFFER_RESET_RELEASE_RESOURCES_BIT));
}

TEST_CASE_METHOD(MeddlFixture, "CommandBufferBadOrder")
{
   init_debugger();
   init_instance();
   init_window();
   init_surface();
   init_device();
   init_command();

   // Can't begin twice
   auto result = _command_buffer->begin();
   REQUIRE(result.has_value());
   result = _command_buffer->begin();
   REQUIRE(result.error() == CommandError::NotReady);

   result = _command_buffer->end();
   REQUIRE(result.has_value());
   result = _command_buffer->end();
   REQUIRE(result.error() == CommandError::NotRecording);

   result = _command_buffer->reset();
   REQUIRE(result.has_value());
   result = _command_buffer->reset();
   REQUIRE(result.error() == CommandError::NotExecutable);
}

TEST_CASE_METHOD(MeddlFixture, "Renderpass")
{
   init_all();
   REQUIRE_NOTHROW([&] {
      REQUIRE(_command_buffer->begin().has_value());

      VkClearValue clear_value = {{{0.0f, 0.0f, 0.0f, 1.0f}}};
      VkRect2D render_area{.offset = {0, 0}, .extent = _swapchain->extent()};

      REQUIRE(_command_buffer
                  ->begin_renderpass(
                      _renderpass.get(), _swapchain.get(), _swapchain->get_framebuffers()[0])
                  .has_value());

      REQUIRE(_command_buffer->set_viewport(defaults::default_viewport(_swapchain->extent()))
                  .has_value());
      REQUIRE(_command_buffer->set_scissor(defaults::default_scissor(_swapchain->extent()))
                  .has_value());
      REQUIRE(_command_buffer->bind_pipeline(_graphics_pipeline.get()).has_value());
      REQUIRE(_command_buffer->draw().has_value());
      REQUIRE(_command_buffer->end_renderpass().has_value());
      REQUIRE(_command_buffer->end().has_value());
   }());
}

TEST_CASE_METHOD(MeddlFixture, "CompileShaders")
{
   init_debugger();
   init_instance();
   init_window();
   init_surface();
   init_device();

   ShaderCompiler compiler;
   auto vertex_spirv =
       compiler.compile(std::filesystem::current_path() / "shader.vert", shaderc_vertex_shader);
   REQUIRE(vertex_spirv.size() == 376);
   std::unique_ptr<ShaderModule> vert_mod;
   REQUIRE_NOTHROW(std::make_unique<ShaderModule>(_device.get(), vertex_spirv));

   auto frag_spirv =
       compiler.compile(std::filesystem::current_path() / "shader.frag", shaderc_fragment_shader);
   REQUIRE(frag_spirv.size() == 143);
   std::unique_ptr<ShaderModule> frag_mod;
   REQUIRE_NOTHROW(std::make_unique<ShaderModule>(_device.get(), frag_spirv));
}
// NOLINTEND
