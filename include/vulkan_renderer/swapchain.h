#pragma once

#include <memory>
#include <optional>
#include <unordered_set>

#include "GLFW/glfw3.h"
#include "vulkan_renderer/device.h"
#include "wrappers/glfw/window.h"
#include "wrappers/vulkan/vulkan_hash.hpp"

namespace meddl::vk {

// Options struct with default value if left unspecified
struct SwapChainOptions {
   VkSurfaceFormatKHR surface_format = {VK_FORMAT_B8G8R8A8_SRGB, VK_COLOR_SPACE_SRGB_NONLINEAR_KHR};
   VkPresentModeKHR present_mode = {VK_PRESENT_MODE_FIFO_KHR};
   std::optional<uint32_t> image_count{};  // TODO: Is this really user friendly?
   uint32_t image_array_layers{1};
};

//! Swapchain representation
class SwapChain {
  public:
   SwapChain() = default;

   SwapChain(LogicalDevice& logical_device,
             const std::unordered_set<VkSurfaceFormatKHR>& formats,
             const std::unordered_set<VkPresentModeKHR>& present_modes,
             const VkSurfaceCapabilitiesKHR& surface_capabilities,
             const VkSwapchainCreateInfoKHR& swapchain_info);

   operator VkSwapchainKHR();

  private:
   std::unordered_set<VkSurfaceFormatKHR> _formats{};
   std::unordered_set<VkPresentModeKHR> _present_modes{};
   VkSurfaceCapabilitiesKHR _surface_capabilities{};
   VkSwapchainCreateInfoKHR _active_swapchain_info{};

   VkSwapchainKHR _handle{VK_NULL_HANDLE};
};

}  // namespace meddl::vk
