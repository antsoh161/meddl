#include "vk/swapchain.h"

#include <iostream>
#include <stdexcept>
#include <vector>

#include "vk/surface.h"

namespace meddl::vk {

//! Helper functions
namespace {
SwapchainDetails get_details(PhysicalDevice* device, Surface* surface)
{
   SwapchainDetails details;
   vkGetPhysicalDeviceSurfaceCapabilitiesKHR(*device, *surface, &details._capabilities);

   uint32_t format_count{};
   vkGetPhysicalDeviceSurfaceFormatsKHR(*device, *surface, &format_count, nullptr);

   std::vector<VkSurfaceFormatKHR> formats{};
   if (format_count > 0) {
      formats.resize(format_count);
      vkGetPhysicalDeviceSurfaceFormatsKHR(*device, *surface, &format_count, formats.data());

      for (const auto& format : formats) {
         details._formats.insert(format);
      }
   }

   uint32_t present_modes_count{};
   vkGetPhysicalDeviceSurfacePresentModesKHR(*device, *surface, &present_modes_count, nullptr);

   std::vector<VkPresentModeKHR> present_modes{};
   if (present_modes_count > 0) {
      present_modes.resize(present_modes_count);
      vkGetPhysicalDeviceSurfacePresentModesKHR(
          *device, *surface, &present_modes_count, present_modes.data());

      for (const auto& present_mode : present_modes) {
         details._present_modes.insert(present_mode);
      }
   }
   return details;
}
};  // namespace


//! New
Swapchain::Swapchain(PhysicalDevice* physical_device,
                           Device* device,
                           Surface* surface,
                           const RenderPass* renderpass,
                           const SwapchainOptions& options,
                           const glfw::FrameBufferSize& fbs)
    : _device(device), _surface(surface)
{
   _details = get_details(physical_device, surface);

   uint32_t min_image_count = std::max(options._image_count, _details._capabilities.minImageCount);
   // 0 means unlimited, so keep above if that's the case
   if (_details._capabilities.maxImageCount > 0) {
      min_image_count = std::min(min_image_count, _details._capabilities.maxImageCount);
   }

   VkSwapchainCreateInfoKHR create_info{};
   create_info.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
   create_info.surface = *surface;
   create_info.minImageCount = min_image_count;

   if (_details._formats.find(options._surface_format) != _details._formats.end()) {
      create_info.imageFormat = options._surface_format.format;
      create_info.imageColorSpace = options._surface_format.colorSpace;
   }
   else {
      // TODO: logger warning
      std::println("Requested formats not available, using default");
      create_info.imageFormat = defaults::DEFAULT_IMAGE_FORMAT;
      create_info.imageColorSpace = defaults::DEFAULT_IMAGE_COLOR_SPACE;
   }

   if (_details._capabilities.currentExtent.width != std::numeric_limits<uint32_t>::max()) {
      _extent2d = _details._capabilities.currentExtent;
   }
   else {
      _extent2d = {std::clamp(fbs.width,
                           _details._capabilities.minImageExtent.width,
                           _details._capabilities.maxImageExtent.width),
                std::clamp(fbs.height,
                           _details._capabilities.minImageExtent.height,
                           _details._capabilities.maxImageExtent.height)};
   }
   create_info.imageExtent = _extent2d;
   create_info.imageArrayLayers = options._image_array_layers;
   create_info.imageUsage = options._image_usage_flags;

   auto queue_families = physical_device->get_queue_families();
   if (queue_families.size() > 1 && false) {
      // TODO: logger debug
      std::println("Swapchain CONCURRENT mode because size: {}", queue_families.size());
      create_info.imageSharingMode = VK_SHARING_MODE_CONCURRENT;
      create_info.queueFamilyIndexCount = queue_families.size();
      std::vector<uint32_t> indicies(queue_families.size());
      std::iota(indicies.begin(), indicies.end(), 0);
      create_info.pQueueFamilyIndices = indicies.data();
   }
   else {
      std::println("Swapchain EXCLUSIVE mode");
      create_info.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
   }

   create_info.preTransform = _details._capabilities.currentTransform;
   create_info.compositeAlpha = defaults::DEFAULT_COMPOSITE_ALPHA_FLAG_BITS;

   if (_details._present_modes.find(options._present_mode) != _details._present_modes.end()) {
      create_info.presentMode = options._present_mode;
   }
   else {
      // TODO: Logger warning
      std::println("Requested present mode not available, using default");
      create_info.presentMode = defaults::DEFAULT_PRESENT_MODE;
   }
   create_info.clipped = options._clipped;
   create_info.oldSwapchain = VK_NULL_HANDLE;
   create_info.pNext = nullptr;

   auto res = vkCreateSwapchainKHR(*_device, &create_info, nullptr, &_swapchain);
   if (res != VK_SUCCESS) {
      throw std::runtime_error{
          std::format("Failed to create swapchain, error: {}", static_cast<int32_t>(res))};
   }
   // TODO: Logger debug
   std::println("Swapchain create with image count: {}", create_info.minImageCount);

   uint32_t image_count{};
   vkGetSwapchainImagesKHR(*_device, _swapchain, &image_count, nullptr);
   _images.resize(image_count);
   vkGetSwapchainImagesKHR(*_device, _swapchain, &image_count, _images.data());
   _image_views.resize(_images.size());

   VkImageViewCreateInfo image_view_info{};

   image_view_info.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
   image_view_info.viewType = VK_IMAGE_VIEW_TYPE_2D;
   image_view_info.format = defaults::DEFAULT_IMAGE_FORMAT;
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
      if (vkCreateImageView(*_device, &image_view_info, nullptr, &_image_views[i]) != VK_SUCCESS) {
         throw std::runtime_error("createImageView failed..");
      }
   }
   _framebuffers.resize(_image_views.size());

   VkFramebufferCreateInfo framebuffer_info{};
   framebuffer_info.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
   framebuffer_info.renderPass = renderpass->vk();
   framebuffer_info.attachmentCount = 1; // Todo: 1?
   framebuffer_info.width = _extent2d.width;
   framebuffer_info.height = _extent2d.height;
   framebuffer_info.layers = 1; // Todo: 1?
   for(size_t i = 0; i < _image_views.size(); i++)
   {
      framebuffer_info.pAttachments = &_image_views.at(i);
      // TODO: Allocator
      if(vkCreateFramebuffer(_device->vk(), &framebuffer_info, nullptr, &_framebuffers.at(i)) != VK_SUCCESS)
      {
         throw std::runtime_error("createFramebuffer failed..");
      }
   }

}

Swapchain::~Swapchain()
{
   if(_swapchain)
   {
      for(auto image_view : _image_views)
      {
         vkDestroyImageView(_device->vk(), image_view, _device->get_allocators());
      }
      vkDestroySwapchainKHR(_device->vk(), _swapchain, _device->get_allocators());
   }
   for(auto framebuffer : _framebuffers)
   {
      vkDestroyFramebuffer(_device->vk(), framebuffer, _device->get_allocators());
   }
}

std::vector<VkImageView>& Swapchain::get_image_views()
{
   return _image_views;
}
}  // namespace meddl::vk
