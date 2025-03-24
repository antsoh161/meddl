
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
#include "engine/render/vk/config.h"
#include "engine/render/vk/defaults.h"
#include "engine/render/vk/descriptor.h"
#include "engine/render/vk/device.h"
#include "engine/render/vk/instance.h"
#include "engine/render/vk/renderpass.h"
#include "engine/render/vk/shared.h"
#include "engine/shader.h"
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
   glfwWindowHint(GLFW_RESIZABLE, GLFW_FALSE);

   auto window =
       std::make_shared<glfw::Window>(_config.window_width, _config.window_height, _config.title);

   // Create and return the renderer
   return {window};
}

constexpr size_t MAX_FRAMES_IN_FLIGHT = 2;
Renderer::Renderer(std::shared_ptr<glfw::Window> window) : _window(std::move(window))
{
   meddl::log::get_logger()->set_level(spdlog::level::debug);
   auto debug_config = vk::DebugConfiguration();
   auto instance_config = vk::InstanceConfiguration();
   // _instance = std::make_unique<vk::Instance>(instance_config, debug_config);
   auto instance = vk::Instance::create(instance_config, debug_config);
   if(instance) {
      _instance = std::move(instance.value());
   }
   // _surface = std::make_unique<vk::Surface>(_window.get(), _instance.get());
   _surface = std::make_unique<render::vk::Surface>(
       render::vk::Surface::create(platform::glfw_window_handle{_window.get()->glfw()},
                                   &_instance)
           .value());

   vk::DevicePicker picker(&_instance, _surface.get());
   auto picked = picker.pick_best(vk::DevicePickerStrategy::HighPerformance);

   _device = std::make_unique<vk::Device>(
       picked.value().best_Device, picked.value().config, _instance.debugger());

   auto graphics_conf = vk::presets::forward_rendering();
   auto validator = vk::ConfigValidator(_device->physical_device(), _surface.get());

   validator.validate_surface_format(graphics_conf);
   validator.validate_renderpass(graphics_conf);

   _renderpass = std::make_unique<vk::RenderPass>(_device.get(), graphics_conf);

   _swapchain = std::make_unique<vk::Swapchain>(_device.get(),
                                                _surface.get(),
                                                _renderpass.get(),
                                                graphics_conf,
                                                _window->get_framebuffer_size());
   //
   auto vert = engine::loader::load_shader(std::filesystem::current_path() / "shader.vert", "main");
   _vert_spirv = vert.value().spirv_code;

   auto frag = engine::loader::load_shader(std::filesystem::current_path() / "shader.frag", "main");
   _frag_spirv = frag.value().spirv_code;

   _frag_mod = std::make_unique<vk::ShaderModule>(_device.get(), _frag_spirv);
   _vert_mod = std::make_unique<vk::ShaderModule>(_device.get(), _vert_spirv);

   const auto bdesc = vk::create_vertex_binding_description(engine::vertex_layout::stride);
   const auto vattr =
       vk::create_vertex_attribute_descriptions(engine::vertex_layout::position_offset,
                                                engine::vertex_layout::color_offset,
                                                engine::vertex_layout::normal_offset,
                                                engine::vertex_layout::texcoord_offset);

   _descriptor_set_layout = std::make_unique<vk::DescriptorSetLayout>(
       _device.get(), graphics_conf.descriptor_layouts.ubo_sampler);

   _pipeline_layout =
       std::make_unique<vk::PipelineLayout>(_device.get(), _descriptor_set_layout.get());
   _graphics_pipeline = std::make_unique<vk::GraphicsPipeline>(_vert_mod.get(),
                                                               _frag_mod.get(),
                                                               _device.get(),
                                                               _pipeline_layout.get(),
                                                               _renderpass.get(),
                                                               bdesc,
                                                               vattr);
   //
   // TODO: 0 is not always the correct index, save somewhere to fetch for the commandpool
   _command_pool = std::make_unique<vk::CommandPool>(
       _device.get(), 0, vk::defaults::DEFAULT_COMMAND_POOL_FLAGS);

   std::ranges::for_each(std::views::iota(0u, MAX_FRAMES_IN_FLIGHT), [this, graphics_conf](auto) {
      _command_buffers.emplace_back(_device.get(), _command_pool.get());
      _image_available.emplace_back(_device.get());
      _render_finished.emplace_back(_device.get());
      auto& buffer = _uniform_buffers.emplace_back(
          _device.get(),
          engine::transform_layout::stride,
          VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
          VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);
      buffer.map();  // TODO: map memory here?

      auto& pool =
          _descriptor_pools.emplace_back(_device.get(), graphics_conf.descriptor_pools.standard);
      _descriptor_sets.emplace_back(
          _device.get(), &pool, _descriptor_set_layout.get());  // todo: refactor
      _fences.emplace_back(_device.get());

      _descriptor_sets.back().update(0, buffer.vk(), 0, sizeof(engine::TransformUBO));
   });
   _instance.log_device_info(_surface.get());
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
      _swapchain = vk::Swapchain::recreate(_device.get(),
                                           _surface.get(),
                                           _renderpass.get(),
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

    constexpr std::array<float, 4> DEBUG_COLOR = {0.1f, 0.1f, 1.0f, 1.0f};
   _instance.debugger()->begin_region(
       &_command_buffers.at(_current_frame), "Frame Rendering", DEBUG_COLOR);
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
   _instance.debugger()->end_region(&_command_buffers.at(_current_frame));

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

      _swapchain = vk::Swapchain::recreate(_device.get(),
                                           _surface.get(),
                                           _renderpass.get(),
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

void Renderer::set_view_matrix(const glm::mat4 view_matrix)
{
   _view_matrix = view_matrix;
   _camera_updated = true;
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

   engine::TransformUBO ubo{};
   ubo.model = glm::mat4(1.0f);

   const auto aspect = static_cast<float>(_swapchain->extent().width) /
                       static_cast<float>(_swapchain->extent().height);

    constexpr float FOV_DEGREES = 45.0f;
    constexpr float NEAR_PLANE = 0.01f;
    constexpr float FAR_PLANE = 100.0f;
   ubo.projection = glm::perspective(glm::radians(FOV_DEGREES), aspect, NEAR_PLANE, FAR_PLANE);
   ubo.projection[1][1] *= -1;  // Flip for Vulkan coordinate system

   if (_camera_updated) {
      ubo.view = _view_matrix;
   }
   else {
       constexpr float CAMERA_DISTANCE = 2.0f;
      ubo.view = glm::lookAt(
          glm::vec3(0.0f, 0.0f, -CAMERA_DISTANCE), glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(0.0f, 1.0f, 0.0f));
   }

   std::memcpy(_uniform_buffers.at(current_image).mapped_data(), &ubo, sizeof(ubo));
}

void Renderer::load_texture(const std::string& path)
{
   try {
      _texture = std::make_unique<vk::Texture>(_device.get(), path);
   }
   catch (const std::exception& e) {
      meddl::log::error("Failed to load texture: {}", e.what());
   }
}

}  // namespace meddl::render
