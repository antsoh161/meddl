#pragma once

#include <vulkan/vulkan_core.h>

#include "core/log.h"
#pragma once

#include <vulkan/vulkan.h>

#include <vector>

namespace meddl::render::vk {

//! A global configuration for now, because the coupling between
//! configurations and components is an ungodly mess in vulkan
//! So far:
//  Image and ImageView Coupling:
//    - Image format must match ImageView format
//    - Image type restricts valid ImageView types (e.g., 2D image only works with 2D view types)
//    - Components swizzling should be consistent
//
// Attachments and Images Coupling:
//    - Attachment formats must match corresponding Image/ImageView formats
//    - Attachment usage (color vs depth) must match Image usage flags
//    - Attachment sample count must match Image sample count
//
// Framebuffer and RenderPass Coupling:
//    - Framebuffer attachments must match RenderPass attachment descriptions
//    - Attachment counts and formats must be consistent

struct GraphicsConfiguration {
   struct AttachmentConfig {
      VkFormat format{VK_FORMAT_UNDEFINED};
      VkSampleCountFlagBits samples{VK_SAMPLE_COUNT_1_BIT};
      VkAttachmentLoadOp load_op{VK_ATTACHMENT_LOAD_OP_CLEAR};
      VkAttachmentStoreOp store_op{VK_ATTACHMENT_STORE_OP_STORE};
      VkAttachmentLoadOp stencil_load_op{VK_ATTACHMENT_LOAD_OP_DONT_CARE};
      VkAttachmentStoreOp stencil_store_op{VK_ATTACHMENT_STORE_OP_DONT_CARE};
      VkImageLayout initial_layout{VK_IMAGE_LAYOUT_UNDEFINED};
      VkImageLayout final_layout{VK_IMAGE_LAYOUT_PRESENT_SRC_KHR};
      bool is_depth_stencil{false};

      VkImageType image_type{VK_IMAGE_TYPE_2D};
      VkImageViewType view_type{VK_IMAGE_VIEW_TYPE_2D};
      VkImageTiling tiling{VK_IMAGE_TILING_OPTIMAL};
      VkMemoryPropertyFlags memory_flags{VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT};
      VkComponentMapping components{VK_COMPONENT_SWIZZLE_IDENTITY,
                                    VK_COMPONENT_SWIZZLE_IDENTITY,
                                    VK_COMPONENT_SWIZZLE_IDENTITY,
                                    VK_COMPONENT_SWIZZLE_IDENTITY};
      VkImageCreateFlags image_flags{0};
      VkImageViewCreateFlags view_flags{0};

      [[nodiscard]] VkImageUsageFlags get_usage_flags() const
      {
         return is_depth_stencil ? VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT
                                 : VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;
      }

      [[nodiscard]] VkImageAspectFlags get_aspect_mask() const
      {
         if (is_depth_stencil) {
            if (format == VK_FORMAT_D24_UNORM_S8_UINT || format == VK_FORMAT_D32_SFLOAT_S8_UINT) {
               return VK_IMAGE_ASPECT_DEPTH_BIT | VK_IMAGE_ASPECT_STENCIL_BIT;
            }
            return VK_IMAGE_ASPECT_DEPTH_BIT;
         }
         else {
            return VK_IMAGE_ASPECT_COLOR_BIT;
         }
      }

      [[nodiscard]] VkAttachmentDescription get_attachment_description() const
      {
         return {.format = format,
                 .samples = samples,
                 .loadOp = load_op,
                 .storeOp = store_op,
                 .stencilLoadOp = stencil_load_op,
                 .stencilStoreOp = stencil_store_op,
                 .initialLayout = initial_layout,
                 .finalLayout = final_layout};
      }
      [[nodiscard]] VkImageCreateInfo get_image_create_info(uint32_t width,
                                                            uint32_t height,
                                                            uint32_t depth = 1) const
      {
         VkExtent3D extent{
             .width = width,
             .height = height,
             .depth = depth,
         };
         return {
             .sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO,
             .flags = image_flags,
             .imageType = image_type,
             .format = format,
             .extent = extent,
             .mipLevels = 1,
             .arrayLayers = 1,
             .samples = samples,
             .tiling = tiling,
             .usage = get_usage_flags(),
             .sharingMode = VK_SHARING_MODE_EXCLUSIVE,
             .initialLayout = VK_IMAGE_LAYOUT_UNDEFINED,
         };
      }

      [[nodiscard]] VkImageViewCreateInfo get_view_create_info(VkImage image) const
      {
         VkImageSubresourceRange range{
             .aspectMask = get_aspect_mask(),
             .baseMipLevel = 0,
             .levelCount = 1,
             .baseArrayLayer = 0,
             .layerCount = 1,
         };
         return {
             .sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO,
             .pNext = nullptr,
             .flags = view_flags,
             .image = image,
             .viewType = view_type,
             .format = format,
             .components = components,
             .subresourceRange = range,
         };
      }

      [[nodiscard]] VkClearValue get_clear_value() const
      {
         VkClearValue clear{};
         if (is_depth_stencil) {
            clear.depthStencil = {.depth = 1.0f, .stencil = 0};
         }
         else {
            clear.color = {{0.0f, 0.0f, 0.0f, 1.0f}};
         }
         return clear;
      }
   };

   struct RenderPassConfiguration {
      std::vector<std::vector<VkAttachmentReference>> color_references{};
      std::vector<VkAttachmentReference> depth_references{};
      std::vector<VkSubpassDescription> subpasses{};
      std::vector<VkSubpassDependency> dependencies{};
      std::vector<std::pair<VkStructureType, void*>> extension_chain{};
      VkRenderPassCreateFlags flags{0};
      bool preserve_contents{false};
      VkAllocationCallbacks* custom_allocator{nullptr};
   };

   struct SwapchainConfiguration {
      VkPresentModeKHR preferred_present_mode{VK_PRESENT_MODE_FIFO_KHR};
      uint32_t min_image_count{2};
      VkSurfaceTransformFlagBitsKHR transform{VK_SURFACE_TRANSFORM_IDENTITY_BIT_KHR};
      VkCompositeAlphaFlagBitsKHR composite_alpha{VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR};
      VkImageUsageFlags image_usage{VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT};
      std::vector<uint32_t> queue_family_indices{};
      void* pNext{nullptr};
      VkAllocationCallbacks* custom_allocator{nullptr};
   };

   struct FramebufferConfiguration {
      uint32_t width{0};
      uint32_t height{0};
      uint32_t layers{1};
      VkFramebufferCreateFlags flags{0};
      VkAllocationCallbacks* custom_allocator{nullptr};
   };

   struct DescriptorSetLayoutConfiguration {
      std::vector<VkDescriptorSetLayoutBinding> bindings{};
      VkDescriptorSetLayoutCreateFlags flags{0};
      VkAllocationCallbacks* custom_allocator{nullptr};
   };

   struct PipelineLayoutConfiguration {
      std::vector<VkDescriptorSetLayout> descriptor_set_layouts{};
      std::vector<VkPushConstantRange> push_constant_ranges{};
      VkPipelineLayoutCreateFlags flags{0};
      VkAllocationCallbacks* custom_allocator{nullptr};
   };

   //! Configurations that apply do multiple components
   struct Shared {
      std::vector<AttachmentConfig> attachments{};
      VkSurfaceFormatKHR surface_format{VK_FORMAT_B8G8R8A8_UNORM,
                                        VK_COLOR_SPACE_SRGB_NONLINEAR_KHR};

      [[nodiscard]] std::vector<VkAttachmentDescription> get_attachment_descriptions() const
      {
         std::vector<VkAttachmentDescription> result;
         result.reserve(attachments.size());
         for (const auto& config : attachments) {
            result.push_back(config.get_attachment_description());
         }
         return result;
      }

      [[nodiscard]] std::vector<VkClearValue> get_clear_values() const
      {
         std::vector<VkClearValue> result;
         result.reserve(attachments.size());
         for (const auto& config : attachments) {
            result.push_back(config.get_clear_value());
         }
         return result;
      }
   };

   // AttachmentConfig attachment_config{};
   RenderPassConfiguration renderpass_config{};
   SwapchainConfiguration swapchain_config{};
   FramebufferConfiguration framebuffer_config{};
   DescriptorSetLayoutConfiguration descriptor_set_layout_config{};
   PipelineLayoutConfiguration pipeline_layout_config{};

   Shared shared{};
};

namespace presets {
// Helper function to create a standard color attachment config
constexpr GraphicsConfiguration::AttachmentConfig make_color_attachment(VkFormat format)
{
   GraphicsConfiguration::AttachmentConfig config{};
   config.format = format;
   config.samples = VK_SAMPLE_COUNT_1_BIT;
   config.load_op = VK_ATTACHMENT_LOAD_OP_CLEAR;
   config.store_op = VK_ATTACHMENT_STORE_OP_STORE;
   config.initial_layout = VK_IMAGE_LAYOUT_UNDEFINED;
   config.final_layout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;
   config.is_depth_stencil = false;
   return config;
}

// Helper function to create a standard depth attachment config
constexpr GraphicsConfiguration::AttachmentConfig make_depth_attachment(VkFormat format)
{
   GraphicsConfiguration::AttachmentConfig config{};
   config.format = format;
   config.samples = VK_SAMPLE_COUNT_1_BIT;
   config.load_op = VK_ATTACHMENT_LOAD_OP_CLEAR;
   config.store_op = VK_ATTACHMENT_STORE_OP_DONT_CARE;
   config.initial_layout = VK_IMAGE_LAYOUT_UNDEFINED;
   config.final_layout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
   config.is_depth_stencil = true;
   return config;
}

// Helper function to create a G-buffer attachment config
constexpr GraphicsConfiguration::AttachmentConfig make_gbuffer_attachment(VkFormat format)
{
   GraphicsConfiguration::AttachmentConfig config{};
   config.format = format;
   config.samples = VK_SAMPLE_COUNT_1_BIT;
   config.load_op = VK_ATTACHMENT_LOAD_OP_CLEAR;
   config.store_op = VK_ATTACHMENT_STORE_OP_STORE;
   config.initial_layout = VK_IMAGE_LAYOUT_UNDEFINED;
   config.final_layout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
   config.is_depth_stencil = false;
   return config;
}

inline GraphicsConfiguration::RenderPassConfiguration make_renderpass(
    const GraphicsConfiguration::Shared& shared)
{
   GraphicsConfiguration::RenderPassConfiguration config;

   if (shared.attachments.empty()) {
      return config;  // Return empty config if no attachments
   }

   config.color_references.emplace_back();
   for (size_t i = 0; i < shared.attachments.size(); ++i) {
      const auto& attachmentConfig = shared.attachments[i];

      if (attachmentConfig.is_depth_stencil) {
         config.depth_references.push_back(
             {static_cast<uint32_t>(i), VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL});
      }
      else {
         config.color_references[0].push_back(
             {static_cast<uint32_t>(i), VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL});
      }
   }

   // Create a single subpass description
   VkSubpassDescription subpass = {};
   subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
   subpass.colorAttachmentCount = static_cast<uint32_t>(config.color_references[0].size());
   subpass.pColorAttachments =
       config.color_references[0].empty() ? nullptr : config.color_references[0].data();
   meddl::log::info("in factory: subpass pAttahcment color size {}, layout: {}",
                    static_cast<int32_t>(subpass.colorAttachmentCount),
                    static_cast<int32_t>(subpass.pColorAttachments->layout));

   // Add depth attachment if one exists
   if (!config.depth_references.empty()) {
      subpass.pDepthStencilAttachment = &config.depth_references[0];
   }

   config.subpasses.push_back(subpass);

   // Add dependencies to ensure proper synchronization
   VkSubpassDependency dependency = {};
   dependency.srcSubpass = VK_SUBPASS_EXTERNAL;
   dependency.dstSubpass = 0;
   dependency.srcStageMask =
       VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT | VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT;
   dependency.dstStageMask =
       VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT | VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT;
   dependency.srcAccessMask = 0;

   VkAccessFlags dstAccessMask = 0;
   if (!config.color_references[0].empty()) {
      dstAccessMask |= VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
   }
   if (!config.depth_references.empty()) {
      dstAccessMask |= VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;
   }
   dependency.dstAccessMask = dstAccessMask;
   config.dependencies.push_back(dependency);
   return config;
}

// Apply forward rendering preset to an existing config
inline void apply_forward_rendering(GraphicsConfiguration& config,
                                    VkFormat color_format = VK_FORMAT_B8G8R8A8_UNORM,
                                    VkFormat depth_format = VK_FORMAT_D32_SFLOAT)
{
   // Color attachment
   config.shared.attachments.push_back(make_color_attachment(color_format));

   // Depth attachment
   config.shared.attachments.push_back(make_depth_attachment(depth_format));

   // Surface format
   config.shared.surface_format = {.format = color_format,
                                   .colorSpace = VK_COLOR_SPACE_SRGB_NONLINEAR_KHR};

   // Swapchain default config
   config.swapchain_config.preferred_present_mode = VK_PRESENT_MODE_FIFO_KHR;
   config.swapchain_config.min_image_count = 3;
   config.swapchain_config.image_usage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;

   config.renderpass_config = make_renderpass(config.shared);
}

// Apply compute-focused preset to an existing config
inline void apply_compute_focused(GraphicsConfiguration& config)
{
   // Compute-focused swapchain config
   config.swapchain_config.preferred_present_mode = VK_PRESENT_MODE_FIFO_KHR;
   config.swapchain_config.image_usage =
       VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_STORAGE_BIT;
}

// Apply deferred rendering preset to an existing config
inline void apply_deferred_rendering(GraphicsConfiguration& config,
                                     VkFormat color_format = VK_FORMAT_B8G8R8A8_SRGB,
                                     VkFormat position_format = VK_FORMAT_R16G16B16A16_SFLOAT,
                                     VkFormat normal_format = VK_FORMAT_R16G16B16A16_SFLOAT,
                                     VkFormat albedo_format = VK_FORMAT_R8G8B8A8_UNORM,
                                     VkFormat depth_format = VK_FORMAT_D32_SFLOAT)
{
   // G-buffer attachments
   config.shared.attachments.push_back(make_gbuffer_attachment(position_format));  // Position
   config.shared.attachments.push_back(make_gbuffer_attachment(normal_format));    // Normal
   config.shared.attachments.push_back(make_gbuffer_attachment(albedo_format));    // Albedo

   // Final color output
   config.shared.attachments.push_back(make_color_attachment(color_format));

   // Depth attachment
   auto depth_attachment = make_depth_attachment(depth_format);
   depth_attachment.final_layout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_READ_ONLY_OPTIMAL;
   config.shared.attachments.push_back(depth_attachment);

   // Surface format
   config.shared.surface_format = {.format = color_format,
                                   .colorSpace = VK_COLOR_SPACE_SRGB_NONLINEAR_KHR};

   // Swapchain config suitable for deferred rendering
   config.swapchain_config.image_usage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;
   config.swapchain_config.min_image_count = 2;
}

// Create commonly used config templates as functions
inline GraphicsConfiguration forward_rendering(VkFormat color_format = VK_FORMAT_B8G8R8A8_UNORM,
                                               VkFormat depth_format = VK_FORMAT_D32_SFLOAT)
{
   GraphicsConfiguration config;
   apply_forward_rendering(config, color_format, depth_format);
   return config;
}

inline GraphicsConfiguration compute_focused()
{
   GraphicsConfiguration config;
   apply_compute_focused(config);
   return config;
}

inline GraphicsConfiguration deferred_rendering(
    VkFormat color_format = VK_FORMAT_B8G8R8A8_SRGB,
    VkFormat position_format = VK_FORMAT_R16G16B16A16_SFLOAT,
    VkFormat normal_format = VK_FORMAT_R16G16B16A16_SFLOAT,
    VkFormat albedo_format = VK_FORMAT_R8G8B8A8_UNORM,
    VkFormat depth_format = VK_FORMAT_D32_SFLOAT)
{
   GraphicsConfiguration config;
   apply_deferred_rendering(
       config, color_format, position_format, normal_format, albedo_format, depth_format);
   return config;
}

}  // namespace presets
}  // namespace meddl::render::vk
