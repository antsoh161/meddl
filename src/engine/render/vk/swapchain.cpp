#include "engine/render/vk/swapchain.h"

#include <algorithm>
#include <iostream>
#include <queue>
#include <ranges>
#include <stdexcept>
#include <vector>

#include "engine/render/vk/shared.h"
#include "engine/render/vk/surface.h"

namespace meddl::render::vk {

Swapchain::Swapchain(Device* device,
                     Surface* surface,
                     const RenderPass* renderpass,
                     const GraphicsConfiguration& config,
                     const glfw::FrameBufferSize& fbs)
    : _device(device), _surface(surface), _config(config)
{
   auto* physical = device->physical_device();
   auto caps = physical->capabilities(surface);

   uint32_t min_image_count = std::max(config.swapchain_config.min_image_count, caps.minImageCount);
   meddl::log::debug("config minimum image {}, capabilitiy minimum: {}",
                     config.swapchain_config.min_image_count,
                     caps.minImageCount);

   // 0 means unlimited, so keep above if that's the case
   if (caps.maxImageCount > 0) {
      min_image_count = std::min(min_image_count, caps.maxImageCount);
   }

   VkSwapchainCreateInfoKHR create_info{};
   create_info.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
   create_info.surface = surface->vk();
   create_info.minImageCount = min_image_count;

   create_info.imageFormat = config.shared.surface_format.format;
   create_info.imageColorSpace = config.shared.surface_format.colorSpace;

   // Set up image extent - we still need logic here for proper sizing
   if (caps.currentExtent.width != std::numeric_limits<uint32_t>::max()) {
      _extent2d = caps.currentExtent;
   }
   else {
      _extent2d = {
          .width = std::clamp(fbs.width, caps.minImageExtent.width, caps.maxImageExtent.width),
          .height = std::clamp(fbs.height, caps.minImageExtent.height, caps.maxImageExtent.height)};
   }
   create_info.imageExtent = _extent2d;

   // Set up image properties directly from config
   create_info.imageArrayLayers = config.framebuffer_config.layers;
   create_info.imageUsage = config.swapchain_config.image_usage;

   // Set up queue families
   const auto queue_families = device->physical_device()->get_queue_families();
   if (!config.swapchain_config.queue_family_indices.empty()) {
      // Use explicit queue family indices from config
      create_info.imageSharingMode = VK_SHARING_MODE_CONCURRENT;
      create_info.queueFamilyIndexCount = config.swapchain_config.queue_family_indices.size();
      create_info.pQueueFamilyIndices = config.swapchain_config.queue_family_indices.data();
   }
   else {
      // Default to exclusive mode
      create_info.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
   }

   // Set up remaining properties directly from config
   create_info.preTransform = config.swapchain_config.transform != 0
                                  ? config.swapchain_config.transform
                                  : caps.currentTransform;

   create_info.compositeAlpha = config.swapchain_config.composite_alpha;
   create_info.presentMode = config.swapchain_config.preferred_present_mode;
   create_info.clipped = true;  // We always want clipping
   create_info.oldSwapchain = VK_NULL_HANDLE;
   create_info.pNext = nullptr;

   auto res = vkCreateSwapchainKHR(
       _device->vk(), &create_info, config.swapchain_config.custom_allocator, &_swapchain);
   if (res != VK_SUCCESS) {
      throw std::runtime_error{
          std::format("Failed to create swapchain, error: {}", static_cast<int32_t>(res))};
   }

   // Get swapchain images
   uint32_t image_count{};
   std::vector<VkImage> swapchain_images{};
   vkGetSwapchainImagesKHR(_device->vk(), _swapchain, &image_count, nullptr);
   swapchain_images.resize(image_count);
   vkGetSwapchainImagesKHR(_device->vk(), _swapchain, &image_count, swapchain_images.data());
   meddl::log::info("Swapchain image count: {}", image_count);

   for (const auto& attachment : config.shared.attachments) {
      if (attachment.is_depth_stencil) {
         _depth_image = Image::create(device, attachment, _extent2d.width, _extent2d.height);
      }
      else {
         if (attachment.format != create_info.imageFormat) {
            meddl::log::warn("attachment format != image format, what does this even mean?");
         }
         for (auto& image : swapchain_images) {
            // swapchain_image_config.format = config.attachment_config.format;
            _images.push_back(Image::create_deferred(image, device, attachment));
         }
      }
   }

   // Create framebuffers
   _framebuffers.resize(_images.size());

   VkFramebufferCreateInfo framebuffer_info{};
   framebuffer_info.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
   framebuffer_info.renderPass = renderpass->vk();
   framebuffer_info.attachmentCount =
       config.shared.get_attachment_descriptions().size();  // matches renderpass
   framebuffer_info.width = _extent2d.width;
   framebuffer_info.height = _extent2d.height;
   framebuffer_info.layers = config.framebuffer_config.layers;

   bool has_depth = _depth_image.has_value();
   meddl::log::debug("Has depth? {}", has_depth);
   for (size_t i = 0; i < _images.size(); i++) {
      std::array<VkImageView, 2> attachments{};

      // Color attachment is always at index 0
      attachments[0] = _images.at(i).view();

      // Add depth attachment if we have one
      if (has_depth) {
         attachments[1] = _depth_image->view();
      }

      framebuffer_info.pAttachments = attachments.data();
      if (vkCreateFramebuffer(_device->vk(),
                              &framebuffer_info,
                              config.framebuffer_config.custom_allocator,
                              &_framebuffers.at(i)) != VK_SUCCESS) {
         throw std::runtime_error("Failed to create framebuffer");
      }
   }
}

Swapchain::~Swapchain()
{
   if (_swapchain) {
      _images.clear();
      vkDestroySwapchainKHR(_device->vk(), _swapchain, _device->get_allocators());
   }
   for (auto framebuffer : _framebuffers) {
      vkDestroyFramebuffer(_device->vk(), framebuffer, _device->get_allocators());
   }
}

std::unique_ptr<Swapchain> Swapchain::recreate(Device* device,
                                               Surface* surface,
                                               const RenderPass* renderpass,
                                               const glfw::FrameBufferSize& fbs,
                                               std::unique_ptr<Swapchain> old_swapchain)
{
   device->wait_idle();
   if (old_swapchain) {
      old_swapchain.reset();
   }
   return std::make_unique<Swapchain>(device, surface, renderpass, old_swapchain->config(), fbs);
}

// std::vector<VkImageView>& Swapchain::get_image_views()
// {
//    return _image_views;
// }
}  // namespace meddl::render::vk
