#include "vulkan_renderer/swapchain.h"

#include <iostream>
#include <vector>

namespace meddl::vulkan {

void SwapChain::populate_swapchain(VkPhysicalDevice device, VkSurfaceKHR surface) {
   VkSurfaceCapabilitiesKHR capabilities;
   vkGetPhysicalDeviceSurfaceCapabilitiesKHR(device, surface, &capabilities);
   uint32_t format_count{};
   vkGetPhysicalDeviceSurfaceFormatsKHR(device, surface, &format_count, nullptr);

   if (format_count > 0) {
      _formats.resize(format_count);
      vkGetPhysicalDeviceSurfaceFormatsKHR(device, surface, &format_count, _formats.data());
   } else {
      std::cerr << "No formats available..\n";
   }

   uint32_t present_modes_count{};
   vkGetPhysicalDeviceSurfacePresentModesKHR(device, surface, &present_modes_count, nullptr);
   if (present_modes_count > 0) {
      _present_modes.resize(present_modes_count);
      vkGetPhysicalDeviceSurfacePresentModesKHR(
          device, surface, &present_modes_count, _present_modes.data());
   } else {
      std::cerr << "No present modes available..\n";
   }
   std::cout << "Populated swapchain with nr formats: " << _formats.size()
             << ", and nr present_modes: " << _present_modes.size() << std::endl;
}
}  // namespace meddl::vulkan
