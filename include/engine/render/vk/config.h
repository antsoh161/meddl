#pragma once

#include "engine/render/vk/physical_device.h"
#include "engine/render/vk/shared.h"
namespace meddl::render::vk {
class ConfigValidator {
  public:
   ConfigValidator(PhysicalDevice* device, Surface* surface);

   bool validate_surface_format(const GraphicsConfiguration& config);
   bool validate_renderpass(const GraphicsConfiguration& config);

  private:
   void suggest_format_alternatives(VkFormat format, bool depth_stencil, VkImageTiling tiling);

   PhysicalDevice* _physical_device;
   Surface* _surface;
};
}  // namespace meddl::render::vk
