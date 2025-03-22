#pragma once

#include <memory>
#include <optional>
#include <stdexcept>

#include "engine/gpu_types.h"
#include "engine/render/vk/vk.h"
#include "engine/window.h"

namespace meddl::render {

struct render_config {
   std::string app_name{"Meddl Leude"};
   uint32_t version = 1;
   int32_t window_width = 240;
   int32_t window_height = 160;
   std::string title{"Meddl Engine"};
   bool enable_debugger{true};
   bool vsync{true};
};

class Renderer;
class RendererBuilder {
  public:
   enum class Platform { Glfw, Web };
   enum class Backend { Vulkan, WebGPU };

   RendererBuilder(Platform platform, Backend backend) : _platform(platform), _backend(backend)
   {
      if (platform != Platform::Glfw || backend != Backend::Vulkan) {
         throw std::invalid_argument("Only glfw + vulkan is supported");
      }
   }

   // Configure the application
   RendererBuilder& app_name(std::string name)
   {
      _config.app_name = std::move(name);
      return *this;
   }

   RendererBuilder& window_size(int width, int height)
   {
      _config.window_width = width;
      _config.window_height = height;
      return *this;
   }

   RendererBuilder& window_title(std::string title)
   {
      _config.title = std::move(title);
      return *this;
   }

   RendererBuilder& validation(bool enable)
   {
      _config.enable_debugger = enable;
      return *this;
   }

   RendererBuilder& vsync(bool enable)
   {
      _config.vsync = enable;
      return *this;
   }

   Renderer build();

  private:
   Renderer make_glfw_vulkan();
   Platform _platform;
   Backend _backend;
   render_config _config;
};

class Renderer {
  public:
   Renderer(std::shared_ptr<glfw::Window> window);
   ~Renderer() = default;

   Renderer(const Renderer&) = delete;
   Renderer& operator=(const Renderer&) = delete;
   Renderer(Renderer&&) = default;
   Renderer& operator=(Renderer&&) = default;

   void set_view_matrix(const glm::mat4 view_matrix);
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
   glm::mat4 _view_matrix = glm::mat4(1.0f);
   bool _camera_updated{false};
};
}  // namespace meddl::render

template <>
struct std::formatter<meddl::render::Renderer> : std::formatter<std::string> {
   auto format(const meddl::render::Renderer& value, std::format_context& ctx) const
   {
      std::string str = std::format("Renderer({})", static_cast<const void*>(&value));
      return std::formatter<std::string>::format(str, ctx);
   }
};
