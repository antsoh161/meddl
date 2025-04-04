#include "engine/render/vk/renderpass.h"

#include <vulkan/vulkan_core.h>

#include "engine/render/vk/shared.h"

namespace meddl::render::vk {

std::expected<RenderPass, error::Error> RenderPass::create(Device* device,
                                                           const GraphicsConfiguration& config)
{
   RenderPass pass;
   pass._device = device;

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

   auto result = vkCreateRenderPass(*pass._device,
                                    &renderpass_info,
                                    config.renderpass_config.custom_allocator,
                                    &pass._render_pass);
   if (result != VK_SUCCESS) {
      return std::unexpected(
          error::Error(std::format("vkCreateRenderPass failed: {}", static_cast<int32_t>(result))));
   }
   return pass;
}

RenderPass::RenderPass(RenderPass&& other) noexcept
    : _render_pass(other._render_pass), _device(other._device)
{
   other._device = nullptr;
   other._render_pass = VK_NULL_HANDLE;
}

RenderPass& RenderPass::operator=(RenderPass&& other) noexcept
{
   if (this != &other) {
      if (_render_pass && _device) {
         vkDestroyRenderPass(_device->vk(), _render_pass, nullptr);
      }
      _device = other._device;
      _render_pass = other._render_pass;
      other._device = nullptr;
      other._render_pass = VK_NULL_HANDLE;
   }
   return *this;
}

RenderPass::~RenderPass()
{
   if (_render_pass && _device) {
      vkDestroyRenderPass(_device->vk(), _render_pass, nullptr);
   }
}

}  // namespace meddl::render::vk
