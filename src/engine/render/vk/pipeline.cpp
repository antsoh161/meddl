#include "engine/render/vk/pipeline.h"

#include <array>
#include <memory>

#include "engine/render/vk/descriptor.h"

namespace meddl::render::vk {

GraphicsPipeline::GraphicsPipeline(
    ShaderModule* vert_shader,
    ShaderModule* frag_shader,
    Device* device,
    PipelineLayout* layout,
    RenderPass* render_pass,
    VkVertexInputBindingDescription binding_description,
    const std::array<VkVertexInputAttributeDescription, 4>& attribute_description)
    : _layout(layout), _device(device)
{
   VkPipelineShaderStageCreateInfo vert_info{};
   vert_info.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
   vert_info.stage = VK_SHADER_STAGE_VERTEX_BIT;
   vert_info.module = vert_shader->vk();
   vert_info.pName = "main";

   VkPipelineShaderStageCreateInfo frag_info{};
   frag_info.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
   frag_info.stage = VK_SHADER_STAGE_FRAGMENT_BIT;
   frag_info.module = frag_shader->vk();
   frag_info.pName = "main";

   std::array<VkPipelineShaderStageCreateInfo, 2> shader_stages = {vert_info, frag_info};

   VkPipelineVertexInputStateCreateInfo vertex_input_info{};
   vertex_input_info.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
   bool has_vertex_info = binding_description.stride > 0;
   if (has_vertex_info) {
      vertex_input_info.vertexBindingDescriptionCount = 1;
      vertex_input_info.pVertexBindingDescriptions = &binding_description;
      vertex_input_info.vertexAttributeDescriptionCount =
          static_cast<uint32_t>(attribute_description.size());
      vertex_input_info.pVertexAttributeDescriptions = attribute_description.data();
   }
   else {
      vertex_input_info.vertexBindingDescriptionCount = 0;
      vertex_input_info.vertexAttributeDescriptionCount = 0;
   }

   VkPipelineInputAssemblyStateCreateInfo input_asm{};
   input_asm.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
   input_asm.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
   input_asm.primitiveRestartEnable = VK_FALSE;

   VkPipelineViewportStateCreateInfo viewport_state{};
   viewport_state.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
   viewport_state.viewportCount = 1;
   viewport_state.scissorCount = 1;

   VkPipelineRasterizationStateCreateInfo rasterizer{};
   rasterizer.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
   rasterizer.depthClampEnable = VK_FALSE;
   rasterizer.rasterizerDiscardEnable = VK_FALSE;
   rasterizer.polygonMode = VK_POLYGON_MODE_FILL;
   rasterizer.lineWidth = 1.0f;
   rasterizer.cullMode = VK_CULL_MODE_BACK_BIT;
   rasterizer.frontFace = VK_FRONT_FACE_COUNTER_CLOCKWISE;
   rasterizer.depthBiasEnable = VK_FALSE;

   VkPipelineDepthStencilStateCreateInfo depth_stencil{};
   depth_stencil.sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO;
   depth_stencil.depthTestEnable = VK_TRUE;
   depth_stencil.depthWriteEnable = VK_TRUE;
   depth_stencil.depthCompareOp = VK_COMPARE_OP_LESS;
   depth_stencil.depthBoundsTestEnable = VK_FALSE;
   depth_stencil.stencilTestEnable = VK_FALSE;

   VkPipelineMultisampleStateCreateInfo multisampling{};
   multisampling.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
   multisampling.sampleShadingEnable = VK_FALSE;
   multisampling.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;

   VkPipelineColorBlendAttachmentState color_blend_attachment{};
   color_blend_attachment.colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT |
                                           VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
   color_blend_attachment.blendEnable = VK_FALSE;

   VkPipelineColorBlendStateCreateInfo color_blending{};
   color_blending.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
   color_blending.logicOpEnable = VK_FALSE;
   color_blending.logicOp = VK_LOGIC_OP_COPY;
   color_blending.attachmentCount = 1;
   color_blending.pAttachments = &color_blend_attachment;
   color_blending.blendConstants[0] = 0.0f;
   color_blending.blendConstants[1] = 0.0f;
   color_blending.blendConstants[2] = 0.0f;
   color_blending.blendConstants[3] = 0.0f;

   std::vector<VkDynamicState> dynamicStates = {VK_DYNAMIC_STATE_VIEWPORT,
                                                VK_DYNAMIC_STATE_SCISSOR};
   VkPipelineDynamicStateCreateInfo dynamic_state{};
   dynamic_state.sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
   dynamic_state.dynamicStateCount = static_cast<uint32_t>(dynamicStates.size());
   dynamic_state.pDynamicStates = dynamicStates.data();

   VkGraphicsPipelineCreateInfo pipeline_info{};
   pipeline_info.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
   pipeline_info.stageCount = 2;
   pipeline_info.pStages = shader_stages.data();
   pipeline_info.pVertexInputState = &vertex_input_info;
   pipeline_info.pInputAssemblyState = &input_asm;
   pipeline_info.pViewportState = &viewport_state;
   pipeline_info.pRasterizationState = &rasterizer;
   pipeline_info.pMultisampleState = &multisampling;
   pipeline_info.pColorBlendState = &color_blending;
   pipeline_info.pDynamicState = &dynamic_state;
   pipeline_info.pDepthStencilState = &depth_stencil;
   pipeline_info.layout = *_layout;
   pipeline_info.renderPass = render_pass->vk();
   pipeline_info.subpass = 0;
   pipeline_info.basePipelineHandle = VK_NULL_HANDLE;

   auto res = vkCreateGraphicsPipelines(
       *_device, VK_NULL_HANDLE, 1, &pipeline_info, _device->get_allocators(), &_pipeline);
   if (res != VK_SUCCESS) {
      throw std::runtime_error{
          std::format("Failed to create graphics pipeline, error: {}", static_cast<int32_t>(res))};
   }
}

GraphicsPipeline::~GraphicsPipeline()
{
   if (_pipeline) {
      vkDestroyPipeline(*_device, _pipeline, _device->get_allocators());
   }
}

PipelineLayout::PipelineLayout(Device* device,
                               const DescriptorSetLayout* dsl,
                               VkPipelineLayoutCreateFlags flags)
    : _device(device)
{
   // TODO: Layouts, push constant ranges
   VkPipelineLayoutCreateInfo create_info{};
   create_info.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
   create_info.flags = flags;
   create_info.pSetLayouts = dsl->vk_ptr();
   create_info.setLayoutCount = 1;
   create_info.pushConstantRangeCount = 0;

   if (vkCreatePipelineLayout(device->vk(), &create_info, device->get_allocators(), &_layout) !=
       VK_SUCCESS) {
      throw std::runtime_error("failed to create pipeline layout!");
   }
}

PipelineLayout::~PipelineLayout()
{
   if (_layout) {
      vkDestroyPipelineLayout(*_device, _layout, _device->get_allocators());
   }
}

}  // namespace meddl::render::vk
