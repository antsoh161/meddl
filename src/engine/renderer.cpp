
#include "engine/renderer.h"

#include <vulkan/vulkan_core.h>

#include <array>
#include <cstring>

#include "engine/vertex.h"
#include "vk/vertex.h"

namespace meddl {
constexpr size_t MAX_FRAMES_IN_FLIGHT = 2;
Renderer::Renderer()
{
   glfwInit();
   glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
   glfwWindowHint(GLFW_RESIZABLE, GLFW_TRUE);
   constexpr auto app_info = meddl::vk::defaults::app_info();
   constexpr auto debug_info = meddl::vk::defaults::debug_info();
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

   const auto bdesc = meddl::vk::create_vertex_binding_description(engine::vertex_layout::stride);
   std::println("Vertex binding stride: {}", bdesc.stride);
   const auto vattr =
       meddl::vk::create_vertex_attribute_descriptions(engine::vertex_layout::position_offset,
                                                       engine::vertex_layout::color_offset,
                                                       engine::vertex_layout::normal_offset,
                                                       engine::vertex_layout::texcoord_offset);
   std::println("Position attribute offset: {}", vattr[0].offset);
   std::println("Color attribute offset: {}", vattr[1].offset);

   _pipeline_layout = std::make_unique<vk::PipelineLayout>(_device.get(), 0);
   _graphics_pipeline = std::make_unique<vk::GraphicsPipeline>(_vert_mod.get(),
                                                               _frag_mod.get(),
                                                               _device.get(),
                                                               _pipeline_layout.get(),
                                                               _renderpass.get(),
                                                               bdesc,
                                                               vattr);

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
   _fences.at(_current_frame).wait(_device.get());
   uint32_t image_index{};
   const auto result = vkAcquireNextImageKHR(_device->vk(),
                                             _swapchain->vk(),
                                             std::numeric_limits<uint64_t>::max(),
                                             _image_available.at(_current_frame).vk(),
                                             VK_NULL_HANDLE,
                                             &image_index);

   if (result == VK_ERROR_OUT_OF_DATE_KHR) {
      _swapchain = vk::Swapchain::recreate(_instance->get_physical_devices().front().get(),
                                           _device.get(),
                                           _surface.get(),
                                           _renderpass.get(),
                                           vk::SwapchainOptions{},
                                           _window->get_framebuffer_size(),
                                           std::move(_swapchain));
      return;
   }
   else if (result != VK_SUCCESS && result != VK_SUBOPTIMAL_KHR) {
      throw std::runtime_error("Failed to acquire next image");
   }
   _fences.at(_current_frame).reset(_device.get());

   _command_buffers.at(_current_frame).reset();
   _command_buffers.at(_current_frame).begin();
   _command_buffers.at(_current_frame)
       .begin_renderpass(
           _renderpass.get(), _swapchain.get(), _swapchain->get_framebuffers()[image_index]);
   _command_buffers.at(_current_frame)
       .set_viewport(vk::defaults::default_viewport(_swapchain->extent()));
   _command_buffers.at(_current_frame)
       .set_scissor(vk::defaults::default_scissor(_swapchain->extent()));
   _command_buffers.at(_current_frame).bind_pipeline(_graphics_pipeline.get());
   if (_vertex_buffer) {
      draw_vertices();
   }
   else {
      _command_buffers.at(_current_frame).draw();
   }
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
           _device->queues().at(0).vk(), 1, &submit_info, _fences.at(_current_frame).vk()) !=
       VK_SUCCESS) {
      throw std::runtime_error("Failed to submit draw command buffer");
   }
   // After vkQueueSubmit
   VkPresentInfoKHR present_info{};
   present_info.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;

   present_info.waitSemaphoreCount = 1;
   present_info.pWaitSemaphores = signal_semaphores.data();

   std::array<VkSwapchainKHR, 1> swapchains = {_swapchain->vk()};
   present_info.swapchainCount = 1;
   present_info.pSwapchains = swapchains.data();
   present_info.pImageIndices = &image_index;

   // Use the presentation queue from your device
   const auto result2 = vkQueuePresentKHR(_device->queues().at(0).vk(), &present_info);
   if (result2 == VK_ERROR_OUT_OF_DATE_KHR || result2 == VK_SUBOPTIMAL_KHR ||
       _window->is_resized()) {
      _window->reset_resized();
      _swapchain = vk::Swapchain::recreate(_instance->get_physical_devices().front().get(),
                                           _device.get(),
                                           _surface.get(),
                                           _renderpass.get(),
                                           vk::SwapchainOptions{},
                                           _window->get_framebuffer_size(),
                                           std::move(_swapchain));
   }
   else if (result2 != VK_SUCCESS) {
      throw std::runtime_error("Failed to present swapchain image");
   }

   // need to be after present because sync?

   _current_frame = (_current_frame + 1) % MAX_FRAMES_IN_FLIGHT;
}

void Renderer::set_vertices(const std::vector<engine::Vertex>& vertices)
{
   const VkDeviceSize buffer_size = vertices.size() * sizeof(engine::Vertex);

   // Create staging buffer with vertex data
   auto staging_buffer = std::make_unique<vk::VertexBuffer>(
       _device.get(),
       buffer_size,
       VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
       VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);

   // Copy vertex data to staging buffer
   staging_buffer->map();
   std::memcpy(staging_buffer->mapped_data(), vertices.data(), buffer_size);
   staging_buffer->unmap();

   // Create vertex buffer
   _vertex_buffer = std::make_unique<vk::VertexBuffer>(
       _device.get(),
       buffer_size,
       VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_VERTEX_BUFFER_BIT,
       VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);

   // Copy from staging buffer to vertex buffer
   _vertex_buffer->copy_from(staging_buffer.get(), buffer_size);
   _vertex_count = static_cast<uint32_t>(vertices.size());
}

void Renderer::draw_vertices(uint32_t vertex_count)
{
   if (vertex_count == 0) {
      vertex_count = _vertex_count;
   }

   if (vertex_count > 0 && _vertex_buffer) {
      VkBuffer vertexBuffers[] = {_vertex_buffer->vk()};
      VkDeviceSize offsets[] = {0};
      vkCmdBindVertexBuffers(
          _command_buffers.at(_current_frame).vk(), 0, 1, vertexBuffers, offsets);
      vkCmdDraw(_command_buffers.at(_current_frame).vk(), vertex_count, 1, 0, 0);
   }
}
}  // namespace meddl
