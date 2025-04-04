#include "engine/render/vk/swapchain.h"

#include <algorithm>
#include <iostream>
#include <queue>
#include <ranges>
#include <stdexcept>
#include <vector>

#include "core/error.h"
#include "engine/render/vk/shared.h"
#include "engine/render/vk/surface.h"

namespace meddl::render::vk {

std::expected<Swapchain, error::Error> Swapchain::create(Device* device,
                                                         Surface* surface,
                                                         const RenderPass* renderpass,
                                                         const GraphicsConfiguration& config,
                                                         const glfw::FrameBufferSize& fbs)
{
   Swapchain swapchain;
   swapchain._device = device;
   swapchain._surface = surface;
   swapchain._config = config;

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
      swapchain._extent2d = caps.currentExtent;
   }
   else {
      swapchain._extent2d = {
          .width = std::clamp(fbs.width, caps.minImageExtent.width, caps.maxImageExtent.width),
          .height = std::clamp(fbs.height, caps.minImageExtent.height, caps.maxImageExtent.height)};
   }
   create_info.imageExtent = swapchain._extent2d;

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

   auto result = vkCreateSwapchainKHR(swapchain._device->vk(),
                                      &create_info,
                                      config.swapchain_config.custom_allocator,
                                      &swapchain._swapchain);
   if (result != VK_SUCCESS) {
      return std::unexpected(error::Error(
          std::format("vkCreateSwapchainKHR failed: {}", static_cast<int32_t>(result))));
   }

   // Get swapchain images
   uint32_t image_count{};
   std::vector<VkImage> swapchain_images{};
   vkGetSwapchainImagesKHR(swapchain._device->vk(), swapchain._swapchain, &image_count, nullptr);
   swapchain_images.resize(image_count);
   vkGetSwapchainImagesKHR(
       swapchain._device->vk(), swapchain._swapchain, &image_count, swapchain_images.data());
   meddl::log::info("Swapchain image count: {}", image_count);

   for (const auto& attachment : config.shared.attachments) {
      if (attachment.is_depth_stencil) {
         swapchain._depth_image = Image::create(
             device, attachment, swapchain._extent2d.width, swapchain._extent2d.height);
      }
      else {
         if (attachment.format != create_info.imageFormat) {
            meddl::log::warn("attachment format != image format, what does this even mean?");
         }
         for (auto& image : swapchain_images) {
            // swapchain_image_config.format = config.attachment_config.format;
            swapchain._images.push_back(Image::create_deferred(image, device, attachment));
         }
      }
   }

   // Create framebuffers
   swapchain._framebuffers.resize(swapchain._images.size());

   VkFramebufferCreateInfo framebuffer_info{};
   framebuffer_info.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
   framebuffer_info.renderPass = renderpass->vk();
   framebuffer_info.attachmentCount =
       config.shared.get_attachment_descriptions().size();  // matches renderpass
   framebuffer_info.width = swapchain._extent2d.width;
   framebuffer_info.height = swapchain._extent2d.height;
   framebuffer_info.layers = config.framebuffer_config.layers;

   bool has_depth = swapchain._depth_image.has_value();
   meddl::log::debug("Has depth? {}", has_depth);
   for (size_t i = 0; i < swapchain._images.size(); i++) {
      std::array<VkImageView, 2> attachments{};

      // Color attachment is always at index 0
      attachments[0] = swapchain._images.at(i).view();

      // Add depth attachment if we have one
      if (has_depth) {
         attachments[1] = swapchain._depth_image->view();
      }

      framebuffer_info.pAttachments = attachments.data();
      result = vkCreateFramebuffer(swapchain._device->vk(),
                                   &framebuffer_info,
                                   config.framebuffer_config.custom_allocator,
                                   &swapchain._framebuffers.at(i));
      if (result != VK_SUCCESS) {
         return std::unexpected(error::Error(
             std::format("vkCreateFramebuffer failed: {}", static_cast<int32_t>(result))));
      }
   }
   return swapchain;
}

Swapchain::Swapchain(Swapchain&& other) noexcept
    : _swapchain(other._swapchain),
      _device(other._device),
      _surface(other._surface),
      _config(other._config),
      _extent2d(other._extent2d),
      _depth_image(std::move(other._depth_image)),
      _images(std::move(other._images)),
      _framebuffers(std::move(other._framebuffers))
{
   // Invalidate the source object without destroying resources
   other._device = nullptr;
   other._surface = nullptr;
   other._swapchain = VK_NULL_HANDLE;
   other._framebuffers.clear();
}

Swapchain& Swapchain::operator=(Swapchain&& other) noexcept
{
   if (this != &other) {
      _device = other._device;
      _surface = other._surface;
      _swapchain = other._swapchain;
      _images = std::move(other._images);
      _depth_image = std::move(other._depth_image);
      _framebuffers = std::move(other._framebuffers);
      _extent2d = other._extent2d;
      _config = other._config;

      other._device = nullptr;
      other._surface = nullptr;
      other._swapchain = VK_NULL_HANDLE;
      other._framebuffers.clear();
   }
   return *this;
}

void Swapchain::deinit()
{
   if (_swapchain) {
      _images.clear();
      vkDestroySwapchainKHR(_device->vk(), _swapchain, _device->get_allocators());
   }
   for (auto framebuffer : _framebuffers) {
      vkDestroyFramebuffer(_device->vk(), framebuffer, _device->get_allocators());
   }
}

Swapchain::~Swapchain()
{
   deinit();
}

std::expected<Swapchain, error::Error> Swapchain::recreate(Device* device,
                                                           Surface* surface,
                                                           const RenderPass* renderpass,
                                                           const glfw::FrameBufferSize& fbs,
                                                           Swapchain& old_swapchain)
{
   device->wait_idle();
   old_swapchain.deinit();
   return Swapchain::create(device, surface, renderpass, old_swapchain.config(), fbs);
}

// std::vector<VkImageView>& Swapchain::get_image_views()
// {
//    return _image_views;
// }
}  // namespace meddl::render::vk
