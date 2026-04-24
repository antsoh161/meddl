#pragma once

#include <vulkan/vulkan_core.h>

#include "core/error.h"
#include "engine/render/vk/descriptor.h"
#include "engine/render/vk/device.h"
#include "engine/render/vk/renderpass.h"
#include "engine/render/vk/shader.h"

namespace meddl::render::vk {

class PipelineLayout {
  public:
   PipelineLayout() = default;
   static std::expected<PipelineLayout, error::Error> create(Device* device,
                                                             const DescriptorSetLayout* dsl,
                                                             VkPipelineLayoutCreateFlags flags = 0);

   ~PipelineLayout();

   PipelineLayout(const PipelineLayout&) = delete;
   PipelineLayout& operator=(const PipelineLayout&) = delete;

   PipelineLayout& operator=(PipelineLayout&&) noexcept;
   PipelineLayout(PipelineLayout&&) noexcept;

   operator VkPipelineLayout() const { return _layout; }
   [[nodiscard]] VkPipelineLayout vk() const { return _layout; }

  private:
   Device* _device{nullptr};
   VkPipelineLayout _layout{VK_NULL_HANDLE};
};

class GraphicsPipeline {
  public:
   GraphicsPipeline() = default;
   static std::expected<GraphicsPipeline, error::Error> create(
       ShaderModule* vert_shader,
       ShaderModule* frag_shader,
       Device* device,
       PipelineLayout* layout,
       RenderPass* render_pass,
       VkVertexInputBindingDescription binding_description,
       const std::array<VkVertexInputAttributeDescription, 4>& attribute_description);
   ~GraphicsPipeline();

   GraphicsPipeline(const GraphicsPipeline&) = delete;
   GraphicsPipeline& operator=(const GraphicsPipeline&) = delete;

   GraphicsPipeline(GraphicsPipeline&&) noexcept;
   GraphicsPipeline& operator=(GraphicsPipeline&&) noexcept;

   [[nodiscard]] VkPipeline vk() const { return _pipeline; }

  private:
   PipelineLayout* _layout{nullptr};
   Device* _device{nullptr};
   VkPipeline _pipeline{VK_NULL_HANDLE};
};
}  // namespace meddl::render::vk
