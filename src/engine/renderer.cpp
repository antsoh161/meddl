
#include "engine/renderer.h"

#include <vulkan/vulkan_core.h>

#include <array>

namespace meddl {
constexpr size_t MAX_FRAMES_IN_FLIGHT = 2;
Renderer::Renderer()
{
   glfwInit();
   glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
   glfwWindowHint(GLFW_RESIZABLE, GLFW_FALSE);
   auto app_info = meddl::vk::defaults::app_info();
   auto debug_info = meddl::vk::defaults::debug_info();
   _debugger = std::make_optional<Debugger>();
   _debugger->add_validation_layer("VK_LAYER_KHRONOS_validation");
   _instance = std::make_unique<vk::Instance>(app_info, debug_info, _debugger);
   _window = std::make_shared<glfw::Window>(
       vk::defaults::DEFAULT_WINDOW_HEIGHT, vk::defaults::DEFAULT_WINDOW_WIDTH, "Test Window");
   _surface = std::make_unique<vk::Surface>(_window.get(), _instance.get());

   std::optional<int> present_index{};
   std::optional<int> graphics_index{};

   for (const auto& device : _instance->get_physical_devices()) {
      present_index = device->get_present_family(_surface.get());
      graphics_index = device->get_queue_family(VK_QUEUE_GRAPHICS_BIT);
      std::println("Present: {}, Graphics: {}", present_index.value(), graphics_index.value());
   }

   auto physical_device = _instance->get_physical_devices().front();
   auto extensions = meddl::vk::defaults::device_extensions();

   auto config = vk::QueueConfiguration(present_index.value());
   std::unordered_map<uint32_t, vk::QueueConfiguration> configs = {
       {graphics_index.value(), config}};
   _device = std::make_unique<vk::Device>(
       physical_device.get(), configs, extensions, std::nullopt, _debugger);

   auto color_attachement = vk::defaults::color_attachment(vk::defaults::DEFAULT_IMAGE_FORMAT);
   _renderpass = std::make_unique<vk::RenderPass>(_device.get(), color_attachement);

   vk::SwapchainOptions options{};
   _swapchain = std::make_unique<vk::Swapchain>(_instance->get_physical_devices().front().get(),
                                                _device.get(),
                                                _surface.get(),
                                                _renderpass.get(),
                                                options,
                                                _window->get_framebuffer_size());

   vk::ShaderCompiler compiler;
   _vert_spirv =
       compiler.compile(std::filesystem::current_path() / "shader.vert", shaderc_vertex_shader);

   _frag_spirv =
       compiler.compile(std::filesystem::current_path() / "shader.frag", shaderc_fragment_shader);

   _frag_mod = std::make_unique<vk::ShaderModule>(_device.get(), _frag_spirv);
   _vert_mod = std::make_unique<vk::ShaderModule>(_device.get(), _vert_spirv);
   std::println(("frag size: {}, vert size: {}"), _frag_spirv.size(), _vert_spirv.size());

   _pipeline_layout = std::make_unique<vk::PipelineLayout>(_device.get(), 0);
   _graphics_pipeline = std::make_unique<vk::GraphicsPipeline>(
       _vert_mod.get(), _frag_mod.get(), _device.get(), _pipeline_layout.get(), _renderpass.get());

   // TODO: 0 is not always the correct index, save somewhere to fetch for the commandpool
   _command_pool = std::make_unique<vk::CommandPool>(
       _device.get(), 0, meddl::vk::defaults::DEFAULT_COMMAND_POOL_FLAGS);

   for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {
      _command_buffers.emplace_back(_device.get(), _command_pool.get());
      _image_available.emplace_back(_device.get());
      _render_finished.emplace_back(_device.get());
      _fences.emplace_back(_device.get());
   }
}

void Renderer::draw()
{
   uint32_t imageIndex{};
   _fences.at(_current_frame).wait(_device.get());
   _fences.at(_current_frame).reset(_device.get());
   vkAcquireNextImageKHR(_device->vk(),
                         _swapchain->vk(),
                         std::numeric_limits<uint64_t>::max(),
                         _image_available.at(_current_frame).vk(),
                         VK_NULL_HANDLE,
                         &imageIndex);

   _command_buffers.at(_current_frame).reset();
   _command_buffers.at(_current_frame).begin();
   _command_buffers.at(_current_frame)
       .begin_renderpass(
           _renderpass.get(), _swapchain.get(), _swapchain->get_framebuffers()[imageIndex]);
   _command_buffers.at(_current_frame)
       .set_viewport(vk::defaults::default_viewport(_swapchain->extent()));
   _command_buffers.at(_current_frame)
       .set_scissor(vk::defaults::default_scissor(_swapchain->extent()));
   _command_buffers.at(_current_frame).bind_pipeline(_graphics_pipeline.get());
   _command_buffers.at(_current_frame).draw();
   _command_buffers.at(_current_frame).end_renderpass();
   _command_buffers.at(_current_frame).end();
   VkSubmitInfo submit_info{};
   submit_info.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;

   std::array<VkSemaphore, MAX_FRAMES_IN_FLIGHT> wait_semaphores = {
       _image_available[_current_frame].vk()};
   std::array<VkPipelineStageFlags, 1> wait_stages = {
       VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT};
   submit_info.waitSemaphoreCount = 1;
   submit_info.pWaitSemaphores = wait_semaphores.data();
   submit_info.pWaitDstStageMask = wait_stages.data();

   VkCommandBuffer command_buffer = _command_buffers.at(_current_frame).vk();
   submit_info.commandBufferCount = 1;
   submit_info.pCommandBuffers = &command_buffer;

   std::array<VkSemaphore, MAX_FRAMES_IN_FLIGHT> signal_semaphores = {
       _render_finished.at(_current_frame).vk()};
   submit_info.signalSemaphoreCount = 1;
   submit_info.pSignalSemaphores = signal_semaphores.data();

   if (vkQueueSubmit(
           _device->_queues.at(0).vk(), 1, &submit_info, _fences.at(_current_frame).vk()) !=
       VK_SUCCESS) {
      throw std::runtime_error("Failed to submit draw command buffer");
   }
   // After vkQueueSubmit
   VkPresentInfoKHR presentInfo{};
   presentInfo.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;

   presentInfo.waitSemaphoreCount = 1;
   presentInfo.pWaitSemaphores = signal_semaphores.data();

   std::array<VkSwapchainKHR, 1> swapchains = {_swapchain->vk()};
   presentInfo.swapchainCount = 1;
   presentInfo.pSwapchains = swapchains.data();
   presentInfo.pImageIndices = &imageIndex;

   // Use the presentation queue from your device
   vkQueuePresentKHR(_device->_queues.at(0).vk(), &presentInfo);
}
}  // namespace meddl
