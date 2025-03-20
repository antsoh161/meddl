#include "engine/render/vk/renderpass.h"

namespace meddl::render::vk {

RenderPass::RenderPass(Device* device, const VkAttachmentDescription& color_attachment)
    : _device(device)
{
   VkAttachmentReference color_attach_ref{};
   color_attach_ref.attachment = 0;
   color_attach_ref.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

   VkSubpassDescription subpass{};
   subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
   subpass.colorAttachmentCount = 1;
   subpass.pColorAttachments = &color_attach_ref;

   VkRenderPassCreateInfo renderpass_info{};
   renderpass_info.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
   renderpass_info.attachmentCount = 1;
   renderpass_info.pAttachments = &color_attachment;
   renderpass_info.subpassCount = 1;
   renderpass_info.pSubpasses = &subpass;

   if (vkCreateRenderPass(*_device, &renderpass_info, nullptr, &_render_pass) != VK_SUCCESS) {
      throw std::runtime_error("failed to create render pass!");
   }
}

RenderPass::~RenderPass()
{
   if (_render_pass) {
      vkDestroyRenderPass(*_device, _render_pass, _device->get_allocators());
   }
}

}  // namespace meddl::render::vk
