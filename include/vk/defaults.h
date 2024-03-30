#pragma once

#include <print>
#include <unordered_set>

#include "GLFW/glfw3.h"

namespace {
constexpr VKAPI_ATTR VkBool32 VKAPI_CALL
debug_cb(VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity,
         VkDebugUtilsMessageTypeFlagsEXT messageType,
         const VkDebugUtilsMessengerCallbackDataEXT* pCallbackData,
         void* pUserData)
{
   std::println("Validation layer: {}", pCallbackData->pMessage);
   return VK_FALSE;
}
}  // namespace
namespace meddl::vk::defaults {

constexpr float DEFAULT_QUEUE_PRIORITY = 1.0f;
constexpr uint32_t DEFAULT_QUEUE_COUNT = 1;
constexpr uint32_t DEFAULT_IMAGE_COUNT = 3;  // Triple buffering
constexpr uint32_t DEFAULT_IMAGE_ARRAY_LAYERS = 1;
constexpr VkFormat DEFAULT_IMAGE_FORMAT = VK_FORMAT_B8G8R8A8_UNORM;
constexpr VkColorSpaceKHR DEFAULT_IMAGE_COLOR_SPACE = VK_COLOR_SPACE_SRGB_NONLINEAR_KHR;
constexpr VkPresentModeKHR DEFAULT_PRESENT_MODE = VK_PRESENT_MODE_FIFO_KHR;
constexpr VkImageUsageFlags DEFAULT_IMAGE_USAGE_FLAGS = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;
constexpr VkCompositeAlphaFlagBitsKHR DEFAULT_COMPOSITE_ALPHA_FLAG_BITS =
    VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
constexpr bool DEFAULT_SWAPCHAIN_CLIPPED = true;

[[maybe_unused]] static const std::unordered_set<std::string> device_extensions()
{
   return {VK_KHR_SWAPCHAIN_EXTENSION_NAME};
}

[[maybe_unused]] static constexpr VkApplicationInfo app_info()
{
   return {VK_STRUCTURE_TYPE_APPLICATION_INFO,
           nullptr,
           "Default Meddl Application",
           VK_MAKE_VERSION(1, 0, 0),
           "No engine",
           VK_MAKE_VERSION(1, 0, 0),
           VK_API_VERSION_1_0};
}

[[maybe_unused]] static constexpr VkAttachmentDescription color_attachment(VkFormat format)
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


[[maybe_unused]] static constexpr VkAttachmentDescription depth_attachment(VkFormat format)
{
   return {0,
           format,
           VK_SAMPLE_COUNT_1_BIT,
           VK_ATTACHMENT_LOAD_OP_CLEAR,
           VK_ATTACHMENT_STORE_OP_STORE,
           VK_ATTACHMENT_LOAD_OP_DONT_CARE,
           VK_ATTACHMENT_STORE_OP_DONT_CARE,
           VK_IMAGE_LAYOUT_UNDEFINED,
           VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL};
}

[[maybe_unused]] static constexpr VkDebugUtilsMessengerCreateInfoEXT debug_info()
{
   return {VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT,
           nullptr,
           0,
           VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT |
               VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT |
               VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT,
           VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT |
               VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT |
               VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT,
           debug_cb,
           nullptr};
}

}  // namespace meddl::vk::defaults
