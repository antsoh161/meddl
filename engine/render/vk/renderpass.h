#pragma once

#include "GLFW/glfw3.h"
#include "core/error.h"
#include "engine/render/vk/device.h"
#include "engine/render/vk/shared.h"

namespace meddl::render::vk {

class RenderPass {
  public:
   RenderPass() = default;
   static std::expected<RenderPass, error::Error> create(Device* device,
                                                         const GraphicsConfiguration& config);

   ~RenderPass();

   RenderPass(const RenderPass&) = delete;
   RenderPass& operator=(const RenderPass&) = delete;
   RenderPass(RenderPass&&) noexcept;
   RenderPass& operator=(RenderPass&&) noexcept;

   operator VkRenderPass() const { return _render_pass; }
   [[nodiscard]] VkRenderPass vk() const { return _render_pass; }

  private:
   VkRenderPass _render_pass{VK_NULL_HANDLE};
   Device* _device{nullptr};
};
}  // namespace meddl::render::vk
