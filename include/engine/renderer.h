#pragma once

#include <memory>
#include <optional>

#include "engine/gpu_types.h"
#include "vk/vk.h"

namespace meddl {

class Renderer {
  public:
   Renderer(std::shared_ptr<glfw::Window> window);
   ~Renderer() = default;

   Renderer(const Renderer&) = delete;
   Renderer& operator=(const Renderer&) = delete;
   Renderer(Renderer&&) = default;
   Renderer& operator=(Renderer&&) = default;

   void set_vertices(const std::vector<engine::Vertex>& vertices);
   void set_indices(const std::vector<uint32_t>& indices);
   void draw_vertices(uint32_t vertex_count = 0);
   void draw();

   std::shared_ptr<glfw::Window> window() { return _window; };

  private:
   void update_uniform_buffer(uint32_t current_image);
   // "core"
   std::unique_ptr<vk::Instance> _instance{};
   std::shared_ptr<glfw::Window> _window{};
   std::unique_ptr<vk::Surface> _surface{};
   std::unique_ptr<vk::Device> _device{};

   // Graphics
   std::unique_ptr<vk::Swapchain> _swapchain{};
   std::unique_ptr<vk::PipelineLayout> _pipeline_layout{};
   std::unique_ptr<vk::RenderPass> _renderpass{};
   std::unique_ptr<vk::GraphicsPipeline> _graphics_pipeline{};
   std::unique_ptr<vk::CommandPool> _command_pool{};
   std::vector<vk::CommandBuffer> _command_buffers{};
   std::unique_ptr<vk::Buffer> _vertex_buffer{};
   std::unique_ptr<vk::Buffer> _index_buffer{};

   std::unique_ptr<vk::DescriptorSetLayout> _descriptor_set_layout{};
   std::vector<vk::Buffer> _uniform_buffers{};
   std::vector<vk::DescriptorPool> _descriptor_pools{};
   std::vector<vk::DescriptorSet> _descriptor_sets{};

   // Shaders
   std::unique_ptr<vk::ShaderModule> _frag_mod{};
   std::unique_ptr<vk::ShaderModule> _vert_mod{};
   std::vector<uint32_t> _frag_spirv{};
   std::vector<uint32_t> _vert_spirv{};

   // Sync
   std::vector<vk::Semaphore> _image_available{};
   std::vector<vk::Semaphore> _render_finished{};
   std::vector<vk::Fence> _fences{};

   size_t _current_frame{0};
   uint32_t _vertex_count{0};
   uint32_t _index_count{0};
};
}  // namespace meddl

template <>
struct std::formatter<meddl::Renderer> : std::formatter<std::string> {
   auto format(const meddl::Renderer& value, std::format_context& ctx) const
   {
      std::string str = std::format("Renderer({})", static_cast<const void*>(&value));
      return std::formatter<std::string>::format(str, ctx);
   }
};
