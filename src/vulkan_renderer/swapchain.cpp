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
   if (vkCreateSwapchainKHR(logical_device, &_active_swapchain_info, nullptr, &_handle) !=
       VK_SUCCESS) {
      std::runtime_error("Failed to create swapchain");
   }
   uint32_t image_count{};
   vkGetSwapchainImagesKHR(logical_device, _handle, &image_count, nullptr);
   _images.resize(image_count);
   vkGetSwapchainImagesKHR(logical_device, _handle, &image_count, _images.data());
   std::cout << "Created " << _images.size() << " images\n";

   _image_views.resize(_images.size());

   // TODO: Options....
   VkImageViewCreateInfo image_view_info{};
   image_view_info.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
   image_view_info.viewType = VK_IMAGE_VIEW_TYPE_2D;
   image_view_info.format = _active_swapchain_info.imageFormat;
   image_view_info.components.r = VK_COMPONENT_SWIZZLE_IDENTITY;
   image_view_info.components.g = VK_COMPONENT_SWIZZLE_IDENTITY;
   image_view_info.components.b = VK_COMPONENT_SWIZZLE_IDENTITY;
   image_view_info.components.a = VK_COMPONENT_SWIZZLE_IDENTITY;
   image_view_info.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
   image_view_info.subresourceRange.baseMipLevel = 0;
   image_view_info.subresourceRange.levelCount = 1;
   image_view_info.subresourceRange.baseArrayLayer = 0;
   image_view_info.subresourceRange.layerCount = 1;

   for (size_t i = 0; i < _images.size(); i++) {
      image_view_info.image = _images[i];
      if (vkCreateImageView(logical_device, &image_view_info, nullptr, &_image_views[i]) != VK_SUCCESS)
      {
         throw std::runtime_error("Failed to create image views");
      }
   }
   std::cout << "Created " << _image_views.size() << " image views!\n";
}

SwapChain::operator VkSwapchainKHR()
{
   return _handle;
}

constexpr size_t SwapChain::get_image_count() const
{
   return _images.size();
}


std::vector<VkImageView>& SwapChain::get_image_views() {
   return _image_views;
}
}  // namespace meddl::vk
