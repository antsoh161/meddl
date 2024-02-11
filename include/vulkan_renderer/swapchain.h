#pragma once

#include <unordered_set>
#include <vector>

#include "GLFW/glfw3.h"

namespace meddl::vulkan {

//! Swapchain representation
class SwapChain {
  public:
   SwapChain() = default;
   // SwapChain(VkPhysicalDevice device, VkSurfaceKHR surface);

   void populate_swapchain(VkPhysicalDevice device, VkSurfaceKHR surface);

  private:
   std::vector<VkSurfaceFormatKHR> _formats{};
   std::vector<VkPresentModeKHR > _present_modes{};
   VkSwapchainKHR _handle{VK_NULL_HANDLE};
};
}  // namespace meddl::vulkan
