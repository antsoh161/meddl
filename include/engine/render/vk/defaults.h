#pragma once

#include <vulkan/vulkan_core.h>

#include <print>
#include <unordered_set>

#include "GLFW/glfw3.h"
#include "core/log.h"

namespace meddl::render::vk::defaults {
//! Window defaults
constexpr uint32_t DEFAULT_WINDOW_WIDTH = 800;
constexpr uint32_t DEFAULT_WINDOW_HEIGHT = 600;

//! Queue Creation
constexpr float DEFAULT_QUEUE_PRIORITY = 1.0f;
constexpr uint32_t DEFAULT_QUEUE_COUNT = 1;
//! Swapchain Creation
constexpr uint32_t DEFAULT_IMAGE_COUNT = 3;  // Triple buffering
constexpr uint32_t DEFAULT_IMAGE_ARRAY_LAYERS = 1;
constexpr VkFormat DEFAULT_IMAGE_FORMAT = VK_FORMAT_B8G8R8A8_UNORM;
constexpr VkColorSpaceKHR DEFAULT_IMAGE_COLOR_SPACE = VK_COLOR_SPACE_SRGB_NONLINEAR_KHR;
constexpr VkPresentModeKHR DEFAULT_PRESENT_MODE = VK_PRESENT_MODE_FIFO_KHR;
constexpr VkImageUsageFlags DEFAULT_IMAGE_USAGE_FLAGS = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;
constexpr VkCompositeAlphaFlagBitsKHR DEFAULT_COMPOSITE_ALPHA_FLAG_BITS =
    VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
constexpr bool DEFAULT_SWAPCHAIN_CLIPPED = true;

//! CommandPool/CommandBuffer creation
constexpr VkCommandPoolCreateFlags DEFAULT_COMMAND_POOL_FLAGS =
    VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
constexpr uint32_t DEFAULT_COMMAND_BUFFER_COUNT = 1;
constexpr VkCommandBufferUsageFlags DEFAULT_BUFFER_USAGE_FLAGS =
    VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;

//! Device Creation
[[maybe_unused]] static constexpr std::unordered_set<std::string> device_extensions()
{
   return {VK_KHR_SWAPCHAIN_EXTENSION_NAME};
}

//! Instance Creation
[[maybe_unused]] static consteval VkApplicationInfo app_info()
{
   return {VK_STRUCTURE_TYPE_APPLICATION_INFO,
           nullptr,
           "Default Meddl Application",
           VK_MAKE_VERSION(1, 0, 0),
           "No engine",
           VK_MAKE_VERSION(1, 0, 0),
           VK_API_VERSION_1_0};
}

//! Renderpass Creation
[[maybe_unused]] static constexpr VkAttachmentDescription color_attachment(const VkFormat& format)
{
   return {0,
           format,
           VK_SAMPLE_COUNT_1_BIT,
           VK_ATTACHMENT_LOAD_OP_CLEAR,
           VK_ATTACHMENT_STORE_OP_STORE,
           VK_ATTACHMENT_LOAD_OP_DONT_CARE,
           VK_ATTACHMENT_STORE_OP_DONT_CARE,
           VK_IMAGE_LAYOUT_UNDEFINED,
           VK_IMAGE_LAYOUT_PRESENT_SRC_KHR};
}

[[maybe_unused]] static constexpr VkAttachmentDescription depth_attachment(const VkFormat& format)
{
   return {.flags = 0,
           .format = format,
           .samples = VK_SAMPLE_COUNT_1_BIT,
           .loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR,
           .storeOp = VK_ATTACHMENT_STORE_OP_STORE,
           .stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE,
           .stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE,
           .initialLayout = VK_IMAGE_LAYOUT_UNDEFINED,
           .finalLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL};
}

[[maybe_unused]] static constexpr VkViewport default_viewport(const VkExtent2D& extent)
{
   return {
       .x = 0.0f,
       .y = 0.0f,
       .width = static_cast<float>(extent.width),
       .height = static_cast<float>(extent.height),
       .minDepth = 0.0f,
       .maxDepth = 1.0f,
   };
}

[[maybe_unused]] static constexpr VkRect2D default_scissor(const VkExtent2D& extent)
{
   return {
       .offset = {0, 0},
       .extent = extent,
   };
}

[[maybe_unused]] static consteval VkDescriptorSetLayoutBinding default_ubo_layout()
{
   return {
       .binding = 0,
       .descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
       .descriptorCount = 1,
       .stageFlags = VK_SHADER_STAGE_VERTEX_BIT,
       .pImmutableSamplers = nullptr,
   };
}

[[maybe_unused]] static consteval VkDescriptorPoolSize default_descriptor_pool_size()
{
   return {
       .type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
       .descriptorCount = 1,
   };
}

}  // namespace meddl::vk::defaults
