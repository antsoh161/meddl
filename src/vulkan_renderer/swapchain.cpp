#include "vulkan_renderer/swapchain.h"

#include <iostream>
#include <stdexcept>
#include <vector>

namespace meddl::vk {

SwapChain::SwapChain(LogicalDevice& logical_device,
                     const std::unordered_set<VkSurfaceFormatKHR>& formats,
                     const std::unordered_set<VkPresentModeKHR>& present_modes,
                     const VkSurfaceCapabilitiesKHR& surface_capabilities,
                     const VkSwapchainCreateInfoKHR& swapchain_info)
    : _formats(formats),
      _present_modes(present_modes),
      _surface_capabilities(surface_capabilities),
      _active_swapchain_info(swapchain_info)
{
   if (vkCreateSwapchainKHR(
           static_cast<VkDevice>(logical_device), &_active_swapchain_info, nullptr, &_handle) !=
       VK_SUCCESS) {
      std::runtime_error("Failed to create swapchain");
   }
}

SwapChain::operator VkSwapchainKHR()
{
   return _handle;
}
}  // namespace meddl::vk
