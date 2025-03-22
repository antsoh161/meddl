#include "engine/render/vk/renderpass.h"

#include <vulkan/vulkan_core.h>

#include "engine/render/vk/shared.h"

namespace meddl::render::vk {

RenderPass::RenderPass(Device* device, const GraphicsConfiguration& config) : _device(device)
{
   std::vector<VkAttachmentDescription> attachments = config.shared.get_attachment_descriptions();
   std::vector<VkSubpassDescription> subpasses = config.renderpass_config.subpasses;
   std::vector<VkSubpassDependency> dependencies = config.renderpass_config.dependencies;

   VkRenderPassCreateInfo renderpass_info{};
   renderpass_info.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
   renderpass_info.flags = config.renderpass_config.flags;
   renderpass_info.attachmentCount = static_cast<uint32_t>(attachments.size());
   renderpass_info.pAttachments = attachments.empty() ? nullptr : attachments.data();
   renderpass_info.subpassCount = static_cast<uint32_t>(subpasses.size());
   renderpass_info.pSubpasses = subpasses.data();
   renderpass_info.dependencyCount = static_cast<uint32_t>(dependencies.size());
   renderpass_info.pDependencies = dependencies.empty() ? nullptr : dependencies.data();

   for (const auto& subpass : subpasses) {
      meddl::log::info("in constructor: subpass pAttahcment color size {}, layout: {}",
                       static_cast<int32_t>(subpass.colorAttachmentCount),
                       static_cast<int32_t>(subpass.pColorAttachments->layout));
   }
   void* pNext = nullptr;
   for (const auto& [type, ptr] : config.renderpass_config.extension_chain) {
      if (pNext == nullptr) {
         renderpass_info.pNext = ptr;
      }
      else {
         *std::bit_cast<void**>(pNext) = ptr;
      }
      pNext = ptr;
   }

   if (vkCreateRenderPass(
           *_device, &renderpass_info, config.renderpass_config.custom_allocator, &_render_pass) !=
       VK_SUCCESS) {
      throw std::runtime_error("Failed to create render pass!");
   }
}

RenderPass::~RenderPass()
{
   if (_render_pass) {
      vkDestroyRenderPass(_device->vk(), _render_pass, nullptr);
   }
}

}  // namespace meddl::render::vk
