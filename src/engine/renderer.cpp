
#include "engine/renderer.h"

#include <vulkan/vulkan_core.h>

#include <algorithm>
#include <array>
#include <cstring>
#include <memory>
#include <optional>
#include <ranges>
#include <utility>

#include "core/log.h"
#include "engine/gpu_types.h"
#include "engine/render/vk/buffer.h"
#include "engine/render/vk/defaults.h"
#include "engine/render/vk/descriptor.h"
#include "glm/glm.hpp"
#include "glm/gtc/matrix_transform.hpp"

namespace meddl::render {

Renderer RendererBuilder::build()
{
   return make_glfw_vulkan();
}

Renderer RendererBuilder::make_glfw_vulkan()
{
   if (!glfwInit()) {
      throw std::runtime_error("Failed to initialize GLFW");
   }

   glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
   glfwWindowHint(GLFW_RESIZABLE, GLFW_TRUE);

   auto window =
       std::make_shared<glfw::Window>(_config.window_width, _config.window_height, _config.title);

   // Create and return the renderer
   return {window};
}

constexpr size_t MAX_FRAMES_IN_FLIGHT = 2;
Renderer::Renderer(std::shared_ptr<glfw::Window> window) : _window(std::move(window))
{
   meddl::log::get_logger()->set_level(spdlog::level::debug);
   constexpr auto app_info = vk::defaults::app_info();
   auto debug_config = vk::DebugConfiguration();
   _instance = std::make_unique<vk::Instance>(app_info, debug_config);
   // _surface = std::make_unique<vk::Surface>(_window.get(), _instance.get());
   _surface = std::make_unique<render::vk::Surface>(
       render::vk::Surface::create(platform::glfw_window_handle{_window.get()->glfw()},
                                   _instance.get())
           .value());

   std::optional<int> present_index{};
   std::optional<int> graphics_index{};

   for (const auto& device : _instance->get_physical_devices()) {
      present_index = device->get_present_family(_surface.get());
      graphics_index = device->get_queue_family(VK_QUEUE_GRAPHICS_BIT);
   }

   auto physical_device = _instance->get_physical_devices().front();
   auto extensions = vk::defaults::device_extensions();

   auto config = vk::QueueConfiguration(present_index.value());
   std::unordered_map<uint32_t, vk::QueueConfiguration> configs = {
       {graphics_index.value(), config}};
   _device = std::make_unique<vk::Device>(physical_device.get(),
                                          configs,
                                          extensions,
                                          std::nullopt,
                                          _instance->debugger()->get_active_validation_layers());

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

   const auto bdesc = vk::create_vertex_binding_description(engine::vertex_layout::stride);
   const auto vattr =
       vk::create_vertex_attribute_descriptions(engine::vertex_layout::position_offset,
                                                engine::vertex_layout::color_offset,
                                                engine::vertex_layout::normal_offset,
                                                engine::vertex_layout::texcoord_offset);

   _descriptor_set_layout =
       std::make_unique<vk::DescriptorSetLayout>(_device.get(), vk::defaults::default_ubo_layout());

   _pipeline_layout =
       std::make_unique<vk::PipelineLayout>(_device.get(), _descriptor_set_layout.get());
   _graphics_pipeline = std::make_unique<vk::GraphicsPipeline>(_vert_mod.get(),
                                                               _frag_mod.get(),
                                                               _device.get(),
                                                               _pipeline_layout.get(),
                                                               _renderpass.get(),
                                                               bdesc,
                                                               vattr);

   // TODO: 0 is not always the correct index, save somewhere to fetch for the commandpool
   _command_pool = std::make_unique<vk::CommandPool>(
       _device.get(), 0, vk::defaults::DEFAULT_COMMAND_POOL_FLAGS);

   std::ranges::for_each(std::views::iota(0u, MAX_FRAMES_IN_FLIGHT), [this](auto) {
      _command_buffers.emplace_back(_device.get(), _command_pool.get());
      _image_available.emplace_back(_device.get());
      _render_finished.emplace_back(_device.get());
      auto& buffer = _uniform_buffers.emplace_back(
          _device.get(),
          engine::transform_layout::stride,
          VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
          VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);
      buffer.map();  // TODO: map memory here?

      const auto pool_sizes = {vk::defaults::default_descriptor_pool_size()};
      auto& pool = _descriptor_pools.emplace_back(_device.get(), 1, pool_sizes);
      _descriptor_sets.emplace_back(
          _device.get(), &pool, _descriptor_set_layout.get());  // todo: refactor
      _fences.emplace_back(_device.get());

      // Update the descriptor set with uniform buffer binding
      VkDescriptorBufferInfo buffer_info{};
      buffer_info.buffer = buffer.vk();
      buffer_info.offset = 0;
      buffer_info.range = sizeof(engine::TransformUBO);

      VkWriteDescriptorSet descriptor_write{};
      descriptor_write.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
      descriptor_write.dstSet = _descriptor_sets.back().vk();
      descriptor_write.dstBinding = 0;
      descriptor_write.dstArrayElement = 0;
      descriptor_write.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
      descriptor_write.descriptorCount = 1;
      descriptor_write.pBufferInfo = &buffer_info;

      vkUpdateDescriptorSets(_device->vk(), 1, &descriptor_write, 0, nullptr);
   });
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

   update_uniform_buffer(_current_frame);

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

   vkCmdBindDescriptorSets(_command_buffers.at(_current_frame).vk(),
                           VK_PIPELINE_BIND_POINT_GRAPHICS,
                           _pipeline_layout->vk(),
                           0,
                           1,
                           _descriptor_sets.at(_current_frame).vk_ptr(),
                           0,
                           nullptr);
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

void Renderer::set_indices(const std::vector<uint32_t>& indices)
{
   const VkDeviceSize buffer_size = indices.size() * sizeof(uint32_t);

   vkDeviceWaitIdle(_device->vk());

   auto staging_buffer = std::make_unique<vk::Buffer>(
       _device.get(),
       buffer_size,
       VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
       VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);

   staging_buffer->map();
   std::memcpy(staging_buffer->mapped_data(), indices.data(), buffer_size);
   staging_buffer->unmap();

   _index_buffer = std::make_unique<vk::Buffer>(
       _device.get(),
       buffer_size,
       VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_INDEX_BUFFER_BIT,
       VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);

   _index_buffer->copy_from(staging_buffer.get(), buffer_size);
   _index_count = static_cast<uint32_t>(indices.size());
}
void Renderer::set_vertices(const std::vector<engine::Vertex>& vertices)
{
   const VkDeviceSize buffer_size = vertices.size() * sizeof(engine::Vertex);

   vkDeviceWaitIdle(_device->vk());

   auto staging_buffer = std::make_unique<vk::Buffer>(
       _device.get(),
       buffer_size,
       VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
       VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);

   staging_buffer->map();
   std::memcpy(staging_buffer->mapped_data(), vertices.data(), buffer_size);
   staging_buffer->unmap();

   _vertex_buffer = std::make_unique<vk::Buffer>(
       _device.get(),
       buffer_size,
       VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_VERTEX_BUFFER_BIT,
       VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);

   _vertex_buffer->copy_from(staging_buffer.get(), buffer_size);
   _vertex_count = static_cast<uint32_t>(vertices.size());
}

void Renderer::draw_vertices(uint32_t vertex_count)
{
   if (vertex_count == 0) {
      vertex_count = _vertex_count;
   }

   if (vertex_count > 0 && _vertex_buffer) {
      std::array<VkBuffer, 1> vertexBuffers = {_vertex_buffer->vk()};
      std::array<VkDeviceSize, 1> offsets = {0};
      vkCmdBindVertexBuffers(
          _command_buffers.at(_current_frame).vk(), 0, 1, vertexBuffers.data(), offsets.data());
      if (_index_buffer && _index_count > 0) {
         vkCmdBindIndexBuffer(_command_buffers.at(_current_frame).vk(),
                              _index_buffer->vk(),
                              0,
                              VK_INDEX_TYPE_UINT32);
         vkCmdDrawIndexed(_command_buffers.at(_current_frame).vk(), _index_count, 1, 0, 0, 0);
      }
      else {
         vkCmdDraw(_command_buffers.at(_current_frame).vk(), vertex_count, 1, 0, 0);
      }
   }
}

void debug_matrix(const glm::mat4& matrix, const std::string& name)
{
   meddl::log::debug("Matrix: {} [", name);
   for (int i = 0; i < 4; ++i) {
      meddl::log::debug("  [{:.3f}, {:.3f}, {:.3f}, {:.3f}]",
                        matrix[0][i],
                        matrix[1][i],
                        matrix[2][i],
                        matrix[3][i]);
   }
   meddl::log::debug("]");
}

void Renderer::update_uniform_buffer(uint32_t current_image)
{
   static auto start_time = std::chrono::high_resolution_clock::now();

   auto current_time = std::chrono::high_resolution_clock::now();
   float time =
       std::chrono::duration<float, std::chrono::seconds::period>(current_time - start_time)
           .count();

   static int frame_count = 0;

   engine::TransformUBO ubo{};
   ubo.model = glm::mat4(1.0f);

   const auto aspect = static_cast<float>(static_cast<float>(_swapchain->extent().width) /
                                          static_cast<float>(_swapchain->extent().width));

   ubo.projection = glm::perspective(glm::radians(45.0f), aspect, 0.1f, 10.0f);
   ubo.projection[1][1] *= -1;
   ubo.view = glm::lookAt(
       glm::vec3(0.0f, 0.0f, -2.0f), glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(0.0f, 1.0f, 0.0f));

   // if (frame_count++ % 60 == 0) {
   //    debug_matrix(ubo.model, "model");
   //    debug_matrix(ubo.view, "view");
   //    debug_matrix(ubo.projection, "projection");
   // }
   frame_count++;
   // Copy data to already mapped uniform buffer
   std::memcpy(_uniform_buffers.at(current_image).mapped_data(), &ubo, sizeof(ubo));
}

}  // namespace meddl::render
