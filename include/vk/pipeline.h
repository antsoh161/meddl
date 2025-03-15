#pragma once

#include "GLFW/glfw3.h"
#include "vk/device.h"
#include "vk/renderpass.h"
#include "vk/shader.h"

namespace meddl::vk {

class PipelineLayout {
  public:
   PipelineLayout(Device* device, VkPipelineLayoutCreateFlags flags);
   ~PipelineLayout();

   PipelineLayout(const PipelineLayout&) = delete;
   PipelineLayout& operator=(const PipelineLayout&) = delete;

   PipelineLayout& operator=(PipelineLayout&&) = default;
   PipelineLayout(PipelineLayout&&) = default;

   operator VkPipelineLayout() const { return _layout; }
   [[nodiscard]] VkPipelineLayout vk() const { return _layout; }

  private:
   Device* _device;
   VkPipelineLayout _layout{VK_NULL_HANDLE};
};

class GraphicsPipeline {
  public:
   GraphicsPipeline(ShaderModule* vert_shader,
                    ShaderModule* frag_shader,
                    Device* device,
                    PipelineLayout* layout,
                    RenderPass* render_pass);
   ~GraphicsPipeline();

   GraphicsPipeline(const GraphicsPipeline&) = delete;
   GraphicsPipeline& operator=(const GraphicsPipeline&) = delete;

   GraphicsPipeline(GraphicsPipeline&&) = default;
   GraphicsPipeline& operator=(GraphicsPipeline&&) = default;

   [[nodiscard]] VkPipeline vk() const { return _pipeline; }

  private:
   PipelineLayout* _layout;
   Device* _device;
   VkPipeline _pipeline{VK_NULL_HANDLE};
};
}  // namespace meddl::vk
