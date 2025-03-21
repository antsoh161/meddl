#pragma once

#include "GLFW/glfw3.h"
#include "engine/render/vk/device.h"

namespace meddl::render::vk {

class RenderPass {
  public:
   RenderPass(Device* device,
              const VkAttachmentDescription& color_attachment,
              const VkAttachmentDescription& depth_attachment);
   ~RenderPass();

   RenderPass(const RenderPass&) = delete;
   RenderPass& operator=(const RenderPass&) = delete;
   RenderPass(RenderPass&&) = default;
   RenderPass& operator=(RenderPass&&) = delete;

   operator VkRenderPass() const { return _render_pass; }
   [[nodiscard]] VkRenderPass vk() const { return _render_pass; }

  private:
   VkRenderPass _render_pass{VK_NULL_HANDLE};
   Device* _device;
};
}  // namespace meddl::render::vk
