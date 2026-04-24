#include "engine/render/vk/config.h"

#include "core/log.h"
#include "engine/render/vk/shared.h"

namespace meddl::render::vk {

ConfigValidator::ConfigValidator(PhysicalDevice* device, Surface* surface)
    : _physical_device(device), _surface(surface)
{
}

bool ConfigValidator::validate_surface_format(const GraphicsConfiguration& config)
{
   auto formats = _physical_device->formats(_surface);
   auto requested_format = config.shared.surface_format;
   auto found_format = std::ranges::find_if(
       formats.begin(), formats.end(), [&requested_format](const VkSurfaceFormatKHR& format) {
          return format.format == requested_format.format &&
                 format.colorSpace == requested_format.colorSpace;
       });

   if (found_format != formats.end()) {
      return true;
   }
   meddl::log::error("Surface format validation error, surface format: {} not found on device",
                     static_cast<int32_t>(config.shared.surface_format.format));
   return false;
}

bool ConfigValidator::validate_renderpass(const GraphicsConfiguration& config)
{
   meddl::log::debug("Validating renderpass...");
   for (const auto& attachment : config.shared.attachments) {
      VkFormatProperties formatProps;
      vkGetPhysicalDeviceFormatProperties(_physical_device->vk(), attachment.format, &formatProps);

      // Check if the format is supported with the specified tiling
      // get_usage_flags() returns either VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT or
      // VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT based on is_depth_stencil
      // We need to check format feature flags, not usage flags
      VkFormatFeatureFlags required_features = attachment.is_depth_stencil
                                                   ? VK_FORMAT_FEATURE_DEPTH_STENCIL_ATTACHMENT_BIT
                                                   : VK_FORMAT_FEATURE_COLOR_ATTACHMENT_BIT;

      if (attachment.tiling == VK_IMAGE_TILING_LINEAR &&
          !(formatProps.linearTilingFeatures & required_features)) {
         meddl::log::error(
             "Attachment format {} not supported with linear tiling and required feature flags. "
             "is_depth_stencil={}",
             static_cast<int32_t>(attachment.format),
             attachment.is_depth_stencil);

         // Suggest supported formats with correct attachment type
         suggest_format_alternatives(
             attachment.format, attachment.is_depth_stencil, attachment.tiling);
         return false;
      }
      else if (attachment.tiling == VK_IMAGE_TILING_OPTIMAL &&
               !(formatProps.optimalTilingFeatures & required_features)) {
         meddl::log::error(
             "Attachment format {} not supported with optimal tiling and required feature flags. "
             "is_depth_stencil={}",
             static_cast<int32_t>(attachment.format),
             attachment.is_depth_stencil);

         // Suggest supported formats with correct attachment type
         suggest_format_alternatives(
             attachment.format, attachment.is_depth_stencil, attachment.tiling);
         return false;
      }
   }
   meddl::log::debug("Formats ok");

   if (!config.renderpass_config.subpasses.empty()) {
      // Check that we have at least one attachment if we have subpasses
      if (config.shared.attachments.empty()) {
         meddl::log::error("Renderpass has subpasses but no attachments are defined");
         return false;
      }

      // Validate attachment references in the subpasses
      for (size_t i = 0; i < config.renderpass_config.subpasses.size(); ++i) {
         const auto& subpass = config.renderpass_config.subpasses[i];

         // Check pipeline bind point is supported
         if (subpass.pipelineBindPoint != VK_PIPELINE_BIND_POINT_GRAPHICS) {
            meddl::log::warn("Subpass {} uses non-graphics bind point which may not be supported",
                             i);
         }

         // Validate color attachment references
         for (uint32_t j = 0; j < subpass.colorAttachmentCount; ++j) {
            if (!subpass.pColorAttachments) {
               meddl::log::error(
                   "Subpass {} has non-zero color attachment count but null pColorAttachments", i);
               return false;
            }

            uint32_t attachment_index = subpass.pColorAttachments[j].attachment;
            if (attachment_index != VK_ATTACHMENT_UNUSED &&
                attachment_index >= config.shared.attachments.size()) {
               meddl::log::error(
                   "Subpass {} refers to color attachment index {} which is out of bounds",
                   i,
                   attachment_index);
               return false;
            }

            if (attachment_index != VK_ATTACHMENT_UNUSED &&
                config.shared.attachments[attachment_index].is_depth_stencil) {
               meddl::log::error(
                   "Subpass {} refers to attachment {} as color but it is configured as "
                   "depth/stencil",
                   i,
                   attachment_index);
               return false;
            }
         }

         // Validate depth attachment references
         if (subpass.pDepthStencilAttachment) {
            uint32_t attachment_index = subpass.pDepthStencilAttachment->attachment;
            if (attachment_index != VK_ATTACHMENT_UNUSED &&
                attachment_index >= config.shared.attachments.size()) {
               meddl::log::error(
                   "Subpass {} refers to depth attachment index {} which is out of bounds",
                   i,
                   attachment_index);
               return false;
            }

            if (attachment_index != VK_ATTACHMENT_UNUSED &&
                !config.shared.attachments[attachment_index].is_depth_stencil) {
               meddl::log::error(
                   "Subpass {} refers to attachment {} as depth but it is not configured as "
                   "depth/stencil",
                   i,
                   attachment_index);
               return false;
            }
         }

         // Validate preserve attachments (if any)
         if (subpass.pPreserveAttachments && subpass.preserveAttachmentCount > 0) {
            for (uint32_t j = 0; j < subpass.preserveAttachmentCount; ++j) {
               uint32_t attachment_index = subpass.pPreserveAttachments[j];
               if (attachment_index >= config.shared.attachments.size()) {
                  meddl::log::error(
                      "Subpass {} refers to preserve attachment index {} which is out of bounds",
                      i,
                      attachment_index);
                  return false;
               }
            }
         }
      }

      // 3. Validate subpass dependencies
      for (size_t i = 0; i < config.renderpass_config.dependencies.size(); ++i) {
         const auto& dependency = config.renderpass_config.dependencies[i];

         // Make sure subpass indices are valid
         if (dependency.srcSubpass != VK_SUBPASS_EXTERNAL &&
             dependency.srcSubpass >= config.renderpass_config.subpasses.size()) {
            meddl::log::error(
                "Dependency {} refers to invalid source subpass {}", i, dependency.srcSubpass);
            return false;
         }

         if (dependency.dstSubpass != VK_SUBPASS_EXTERNAL &&
             dependency.dstSubpass >= config.renderpass_config.subpasses.size()) {
            meddl::log::error(
                "Dependency {} refers to invalid destination subpass {}", i, dependency.dstSubpass);
            return false;
         }

         // Validate dependency ordering
         if (dependency.srcSubpass != VK_SUBPASS_EXTERNAL &&
             dependency.dstSubpass != VK_SUBPASS_EXTERNAL &&
             dependency.srcSubpass >= dependency.dstSubpass) {
            meddl::log::warn(
                "Dependency {} has source subpass ({}) >= destination subpass ({}), "
                "which may cause unnecessary pipeline stalls",
                i,
                dependency.srcSubpass,
                dependency.dstSubpass);
         }

         // Check for missing stage or access mask flags
         if (dependency.srcStageMask == 0) {
            meddl::log::warn("Dependency {} has zero source stage mask", i);
         }

         if (dependency.dstStageMask == 0) {
            meddl::log::warn("Dependency {} has zero destination stage mask", i);
         }
      }

      // 4. Check for potential performance issues with attachment layouts
      for (size_t i = 0; i < config.shared.attachments.size(); ++i) {
         const auto& attachment = config.shared.attachments[i];

         // Warn about potential performance issues with attachment layout transitions
         if (attachment.initial_layout == VK_IMAGE_LAYOUT_UNDEFINED &&
             attachment.load_op != VK_ATTACHMENT_LOAD_OP_CLEAR &&
             attachment.load_op != VK_ATTACHMENT_LOAD_OP_DONT_CARE) {
            meddl::log::warn(
                "Attachment {} uses UNDEFINED initial layout but has LOAD_OP_LOAD, "
                "which may lead to undefined behavior",
                i);
         }

         // Check for appropriate final layouts based on usage
         if (attachment.is_depth_stencil &&
             attachment.final_layout != VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL &&
             attachment.final_layout != VK_IMAGE_LAYOUT_DEPTH_STENCIL_READ_ONLY_OPTIMAL) {
            meddl::log::warn(
                "Depth/stencil attachment {} has final layout {}, "
                "which may not be optimal for depth/stencil use",
                i,
                static_cast<int32_t>(attachment.final_layout));
         }
      }
   }
   meddl::log::debug("Subpasses ok");
   return true;
}

void ConfigValidator::suggest_format_alternatives(VkFormat format,
                                                  bool depth_stencil,
                                                  VkImageTiling tiling)
{
   meddl::log::info("Suggesting alternatives for format {} ({}):",
                    static_cast<int32_t>(format),
                    depth_stencil ? "depth/stencil" : "color");

   //! VERY arbitrary but these seems very common
   std::vector<VkFormat> candidates;
   if (depth_stencil) {
      candidates = {VK_FORMAT_D32_SFLOAT,
                    VK_FORMAT_D32_SFLOAT_S8_UINT,
                    VK_FORMAT_D24_UNORM_S8_UINT,
                    VK_FORMAT_D16_UNORM};
   }
   else {
      candidates = {VK_FORMAT_B8G8R8A8_UNORM,
                    VK_FORMAT_R8G8B8A8_UNORM,
                    VK_FORMAT_B8G8R8A8_SRGB,
                    VK_FORMAT_R8G8B8A8_SRGB};
   }

   for (auto candidate_format : candidates) {
      if (candidate_format == format) continue;

      VkFormatProperties formatProps;
      vkGetPhysicalDeviceFormatProperties(_physical_device->vk(), candidate_format, &formatProps);

      VkFormatFeatureFlags required = depth_stencil ? VK_FORMAT_FEATURE_DEPTH_STENCIL_ATTACHMENT_BIT
                                                    : VK_FORMAT_FEATURE_COLOR_ATTACHMENT_BIT;

      bool is_supported = false;
      if (tiling == VK_IMAGE_TILING_LINEAR) {
         is_supported = (formatProps.linearTilingFeatures & required) != 0;
      }
      else {  // VK_IMAGE_TILING_OPTIMAL
         is_supported = (formatProps.optimalTilingFeatures & required) != 0;
      }

      if (is_supported) {
         meddl::log::info("  Format {} is supported as an alternative",
                          static_cast<int32_t>(candidate_format));
      }
   }
}

}  // namespace meddl::render::vk
