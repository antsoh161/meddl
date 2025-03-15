#include <catch2/catch_test_macros.hpp>
#include <print>

#include "vk/async.h"
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
using namespace meddl::error;

// NOLINTBEGIN (cppcoreguidelines-avoid-do-while)
namespace {
struct Syncs {
   void init(Device* device)
   {
      _fence = std::make_unique<Fence>(device);
      _image_available = std::make_unique<Semaphore>(device);
      _renderFinished = std::make_unique<Semaphore>(device);
   }
   std::unique_ptr<Fence> _fence{};
   std::unique_ptr<Semaphore> _image_available{};
   std::unique_ptr<Semaphore> _renderFinished{};
};
}  // namespace

class MeddlFixture {
  public:
   ~MeddlFixture()
   {
      if (_device) {
         vkDeviceWaitIdle(_device->vk());
      }
   }
   void init_instance()
   {
      glfwInit();
      glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
      glfwWindowHint(GLFW_RESIZABLE, GLFW_FALSE);
#ifdef CI
      glfwWindowHint(GLFW_VISIBLE, GLFW_FALSE);  // Headless for tests
#endif
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
      _vert_spirv =
          compiler.compile(std::filesystem::current_path() / "shader.vert", shaderc_vertex_shader);

      _frag_spirv = compiler.compile(std::filesystem::current_path() / "shader.frag",
                                     shaderc_fragment_shader);

      _frag_mod = std::make_unique<ShaderModule>(_device.get(), _frag_spirv);
      _vert_mod = std::make_unique<ShaderModule>(_device.get(), _vert_spirv);
      std::println(("frag size: {}, vert size: {}"), _frag_spirv.size(), _vert_spirv.size());

      _pipeline_layout = std::make_unique<PipelineLayout>(_device.get(), 0);
      _graphics_pipeline = std::make_unique<GraphicsPipeline>(_vert_mod.get(),
                                                              _frag_mod.get(),
                                                              _device.get(),
                                                              _pipeline_layout.get(),
                                                              _renderpass.get());
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
      _syncs.init(_device.get());
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
   Syncs _syncs{};
   std::unique_ptr<Swapchain> _swapchain{};
   std::unique_ptr<PipelineLayout> _pipeline_layout{};
   std::unique_ptr<RenderPass> _renderpass{};
   std::unique_ptr<GraphicsPipeline> _graphics_pipeline{};
   std::unique_ptr<CommandPool> _command_pool{};
   std::unique_ptr<CommandBuffer> _command_buffer{};

   std::unique_ptr<ShaderModule> _frag_mod{};
   std::unique_ptr<ShaderModule> _vert_mod{};
   std::vector<uint32_t> _frag_spirv{};
   std::vector<uint32_t> _vert_spirv{};
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
   REQUIRE(result.error() == VulkanError::CommandBufferNotReady);

   result = _command_buffer->end();
   REQUIRE(result.has_value());
   result = _command_buffer->end();
   REQUIRE(result.error() == VulkanError::CommandBufferNotRecording);

   result = _command_buffer->reset();
   REQUIRE(result.has_value());
   result = _command_buffer->reset();
   REQUIRE(result.error() == VulkanError::CommandBufferNotExecutable);
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
   REQUIRE_NOTHROW(std::make_unique<ShaderModule>(_device.get(), vertex_spirv));

   auto frag_spirv =
       compiler.compile(std::filesystem::current_path() / "shader.frag", shaderc_fragment_shader);
   REQUIRE(frag_spirv.size() == 143);
   REQUIRE_NOTHROW(std::make_unique<ShaderModule>(_device.get(), frag_spirv));
}

TEST_CASE_METHOD(MeddlFixture, "Renderloop")
{
   init_all();
   uint32_t imageIndex{};
   for (auto i = 0; i < 200; i++) {
      _syncs._fence->wait(_device.get());
      _syncs._fence->reset(_device.get());
      vkAcquireNextImageKHR(_device->vk(),
                            _swapchain->vk(),
                            std::numeric_limits<uint64_t>::max(),
                            _syncs._image_available->vk(),
                            VK_NULL_HANDLE,
                            &imageIndex);

      _command_buffer->reset();
      _command_buffer->begin();
      _command_buffer->begin_renderpass(
          _renderpass.get(), _swapchain.get(), _swapchain->get_framebuffers()[imageIndex]);
      _command_buffer->set_viewport(defaults::default_viewport(_swapchain->extent()));
      _command_buffer->set_scissor(defaults::default_scissor(_swapchain->extent()));
      _command_buffer->bind_pipeline(_graphics_pipeline.get());
      _command_buffer->draw();
      _command_buffer->end_renderpass();
      _command_buffer->end();
      VkSubmitInfo submit_info{};
      submit_info.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;

      VkSemaphore wait_semaphores[] = {_syncs._image_available->vk()};
      VkPipelineStageFlags wait_stages[] = {VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT};
      submit_info.waitSemaphoreCount = 1;
      submit_info.pWaitSemaphores = wait_semaphores;
      submit_info.pWaitDstStageMask = wait_stages;

      VkCommandBuffer command_buffer = _command_buffer->vk();
      submit_info.commandBufferCount = 1;
      submit_info.pCommandBuffers = &command_buffer;

      VkSemaphore signal_semaphores[] = {_syncs._renderFinished->vk()};
      submit_info.signalSemaphoreCount = 1;
      submit_info.pSignalSemaphores = signal_semaphores;

      if (vkQueueSubmit(_device->_queues.at(0).vk(), 1, &submit_info, _syncs._fence->vk()) !=
          VK_SUCCESS) {
         throw std::runtime_error("Failed to submit draw command buffer");
      }
      // After vkQueueSubmit
      VkPresentInfoKHR presentInfo{};
      presentInfo.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;

      presentInfo.waitSemaphoreCount = 1;
      presentInfo.pWaitSemaphores = signal_semaphores;

      VkSwapchainKHR swapChains[] = {_swapchain->vk()};
      presentInfo.swapchainCount = 1;
      presentInfo.pSwapchains = swapChains;
      presentInfo.pImageIndices = &imageIndex;

      // Use the presentation queue from your device
      vkQueuePresentKHR(_device->_queues.at(0).vk(), &presentInfo);
   }
}
// NOLINTEND
