
#include "vk/pipeline.h"

#include <array>

namespace meddl::vk {

GraphicsPipeline::GraphicsPipeline(const ShaderModule& vert_shader,
                                   const ShaderModule& frag_shader,
                                   Device* device)
    : _device(device)
{
   VkPipelineShaderStageCreateInfo vert_info{};
   vert_info.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
   vert_info.stage = VK_SHADER_STAGE_VERTEX_BIT;
   vert_info.module = vert_shader;
   vert_info.pName = "main";

   VkPipelineShaderStageCreateInfo frag_info{};
   frag_info.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
   frag_info.stage = VK_SHADER_STAGE_FRAGMENT_BIT;
   frag_info.module = frag_shader;
   frag_info.pName = "main";

   std::array<VkPipelineShaderStageCreateInfo, 2> shader_stages = {vert_info, frag_info};
}
}  // namespace meddl::vk
