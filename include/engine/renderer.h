#pragma once

#include <memory>
#include <optional>

#include "vk/vk.h"

namespace meddl {

class Renderer {
  public:
   Renderer();
   ~Renderer() = default;

   Renderer(const Renderer&) = delete;
   Renderer& operator=(const Renderer&) = delete;
   Renderer(Renderer&&) = default;
   Renderer& operator=(Renderer&&) = default;

   void draw();

   std::shared_ptr<glfw::Window> window() { return _window; };

  private:
   // "core"
   std::optional<Debugger> _debugger{};
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
};
}  // namespace meddl
