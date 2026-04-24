#include "engine/render/vk/image.h"

#include <cmath>
#include <stdexcept>

#include "engine/render/vk/buffer.h"
#include "engine/render/vk/command.h"
#include "engine/render/vk/shared.h"
#include "vulkan/vulkan_core.h"

namespace meddl::render::vk {

Image::Image(Device* device, const GraphicsConfiguration::AttachmentConfig& config)
    : _device(device), _config(config), _current_layout(config.initial_layout)
{
}

Image Image::create(Device* device,
                    const GraphicsConfiguration::AttachmentConfig& config,
                    uint32_t width,
                    uint32_t height)
{
   Image result(device, config);
   result._extent = {.width = width, .height = height, .depth = 1};
   auto image_info = config.get_image_create_info(width, height);

   if (vkCreateImage(device->vk(), &image_info, device->get_allocators(), &result._image) !=
       VK_SUCCESS) {
      meddl::log::error("Failed to create image, GG");
   }

   // Get memory requirements
   VkMemoryRequirements mem_requirements;
   vkGetImageMemoryRequirements(device->vk(), result._image, &mem_requirements);
   uint32_t type_index{0};

   auto mem_properties = device->physical_device()->get_memory_properties();

   for (uint32_t i = 0; i < mem_properties.memoryTypeCount; i++) {
      if ((mem_requirements.memoryTypeBits & (1 << i)) &&
          (mem_properties.memoryTypes[i].propertyFlags & config.memory_flags) ==
              config.memory_flags) {
         type_index = i;
         break;
      }
   }

   // Allocate memory
   VkMemoryAllocateInfo alloc_info{};
   alloc_info.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
   alloc_info.allocationSize = mem_requirements.size;
   alloc_info.memoryTypeIndex = type_index;

   if (vkAllocateMemory(device->vk(), &alloc_info, device->get_allocators(), &result._memory) !=
       VK_SUCCESS) {
      meddl::log::error("Failed to create allocate image memory, GG");
   }
   if (vkBindImageMemory(device->vk(), result._image, result._memory, 0) != VK_SUCCESS) {
      meddl::log::error("Failed to bind image memory, GG");
   }

   result._owned_resources = Owned{result._memory};

   result.populate_image_view();

   return result;
}

Image Image::create_deferred(VkImage image,
                             Device* device,
                             const GraphicsConfiguration::AttachmentConfig& config)
{
   Image result(device, config);
   result._image = image;
   result.populate_image_view();
   meddl::log::info("Created a deferred image");
   return result;
}

Image Image::create_texture(Device* device,
                            uint32_t width,
                            uint32_t height,
                            VkFormat format,
                            uint32_t mip_levels,
                            VkImageUsageFlags usage)
{
   GraphicsConfiguration::AttachmentConfig config{};
   config.format = format;
   config.samples = VK_SAMPLE_COUNT_1_BIT;
   config.initial_layout = VK_IMAGE_LAYOUT_UNDEFINED;
   config.final_layout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
   config.memory_flags = VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT;

   // Create an image
   Image result(device, config);
   result._type = ImageType::Texture;
   result._extent = {.width = width, .height = height, .depth = 1};

   // Create image with mip-mapping support
   VkImageCreateInfo imageInfo{};
   imageInfo.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
   imageInfo.imageType = VK_IMAGE_TYPE_2D;
   imageInfo.extent.width = width;
   imageInfo.extent.height = height;
   imageInfo.extent.depth = 1;
   imageInfo.mipLevels = mip_levels;
   imageInfo.arrayLayers = 1;
   imageInfo.format = format;
   imageInfo.tiling = VK_IMAGE_TILING_OPTIMAL;
   imageInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
   imageInfo.usage = usage;
   imageInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
   imageInfo.samples = VK_SAMPLE_COUNT_1_BIT;
   imageInfo.flags = 0;

   if (vkCreateImage(device->vk(), &imageInfo, device->get_allocators(), &result._image) !=
       VK_SUCCESS) {
      meddl::log::error("Failed to create texture image!");
      throw std::runtime_error("Failed to create texture image!");
   }

   // Get memory requirements
   VkMemoryRequirements mem_requirements;
   vkGetImageMemoryRequirements(device->vk(), result._image, &mem_requirements);
   uint32_t type_index{0};

   auto mem_properties = device->physical_device()->get_memory_properties();

   for (uint32_t i = 0; i < mem_properties.memoryTypeCount; i++) {
      if ((mem_requirements.memoryTypeBits & (1 << i)) &&
          (mem_properties.memoryTypes[i].propertyFlags & config.memory_flags) ==
              config.memory_flags) {
         type_index = i;
         break;
      }
   }

   // Allocate memory
   VkMemoryAllocateInfo alloc_info{};
   alloc_info.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
   alloc_info.allocationSize = mem_requirements.size;
   alloc_info.memoryTypeIndex = type_index;

   if (vkAllocateMemory(device->vk(), &alloc_info, device->get_allocators(), &result._memory) !=
       VK_SUCCESS) {
      meddl::log::error("Failed to allocate texture image memory!");
      throw std::runtime_error("Failed to allocate texture image memory!");
   }

   if (vkBindImageMemory(device->vk(), result._image, result._memory, 0) != VK_SUCCESS) {
      meddl::log::error("Failed to bind texture image memory!");
      throw std::runtime_error("Failed to bind texture image memory!");
   }

   result._owned_resources = Owned{result._memory};

   // Create image view with proper mipmap levels
   VkImageViewCreateInfo viewInfo{};
   viewInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
   viewInfo.image = result._image;
   viewInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
   viewInfo.format = format;
   viewInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
   viewInfo.subresourceRange.baseMipLevel = 0;
   viewInfo.subresourceRange.levelCount = mip_levels;
   viewInfo.subresourceRange.baseArrayLayer = 0;
   viewInfo.subresourceRange.layerCount = 1;

   if (vkCreateImageView(device->vk(), &viewInfo, device->get_allocators(), &result._image_view) !=
       VK_SUCCESS) {
      meddl::log::error("Failed to create texture image view!");
      throw std::runtime_error("Failed to create texture image view!");
   }

   return result;
}

Image::~Image()
{
   // Looks yanky, but deferred images are cleaned up by the swapchain
   if (_owned_resources.has_value()) {
      if (_image) {
         vkDestroyImage(_device->vk(), _image, _device->get_allocators());
      }
      if (_owned_resources.value().memory) {
         vkFreeMemory(_device->vk(), _owned_resources.value().memory, _device->get_allocators());
      }
   }
   if (_image_view) {
      vkDestroyImageView(_device->vk(), _image_view, _device->get_allocators());
   }
}

Image::Image(Image&& other) noexcept
    : _device(other._device),
      _config(other._config),
      _image(other._image),
      _image_view(other._image_view),
      _current_layout(other._current_layout),
      _memory(other._memory),
      _owned_resources(std::move(other._owned_resources))
{
   other._memory = VK_NULL_HANDLE;
   other._image = VK_NULL_HANDLE;
   other._image_view = VK_NULL_HANDLE;
   other._memory = VK_NULL_HANDLE;
   other._owned_resources.reset();
}

Image& Image::operator=(Image&& other) noexcept
{
   if (this != &other) {
      // Move resources from other
      _device = other._device;
      _image = other._image;
      _image_view = other._image_view;
      _memory = other._memory;
      _config = other._config;
      _current_layout = other._current_layout;
      _owned_resources = std::move(other._owned_resources);

      other._memory = VK_NULL_HANDLE;
      other._image = VK_NULL_HANDLE;
      other._image_view = VK_NULL_HANDLE;
      other._memory = VK_NULL_HANDLE;
      other._owned_resources.reset();
   }
   return *this;
}

void Image::transition(CommandPool* pool, VkImageLayout old_layout, VkImageLayout new_layout)
{
   auto cmd_buffer = CommandBuffer::begin_one_time_submit(_device, pool);
   if (!cmd_buffer) {
      throw std::runtime_error(std::format("{}", cmd_buffer.error().full_message()));
   }

   VkImageMemoryBarrier barrier{};
   barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
   barrier.oldLayout = old_layout;
   barrier.newLayout = new_layout;
   barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
   barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
   barrier.image = _image;

   // For depth/stencil images, set the aspect mask accordingly
   if (new_layout == VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL) {
      barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT;

      if (_config.is_depth_stencil) {
         barrier.subresourceRange.aspectMask |= VK_IMAGE_ASPECT_STENCIL_BIT;
      }
   }
   else {
      barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
   }

   barrier.subresourceRange.baseMipLevel = 0;
   barrier.subresourceRange.levelCount = VK_REMAINING_MIP_LEVELS;
   barrier.subresourceRange.baseArrayLayer = 0;
   barrier.subresourceRange.layerCount = 1;

   VkPipelineStageFlags sourceStage{};
   VkPipelineStageFlags destinationStage{};

   if (old_layout == VK_IMAGE_LAYOUT_UNDEFINED &&
       new_layout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL) {
      barrier.srcAccessMask = 0;
      barrier.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;

      sourceStage = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
      destinationStage = VK_PIPELINE_STAGE_TRANSFER_BIT;
   }
   else if (old_layout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL &&
            new_layout == VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL) {
      barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
      barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;

      sourceStage = VK_PIPELINE_STAGE_TRANSFER_BIT;
      destinationStage = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
   }
   else if (old_layout == VK_IMAGE_LAYOUT_UNDEFINED &&
            new_layout == VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL) {
      barrier.srcAccessMask = 0;
      barrier.dstAccessMask = VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_READ_BIT |
                              VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;

      sourceStage = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
      destinationStage = VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT;
   }
   else {
      throw std::invalid_argument("Unsupported layout transition!");
   }

   vkCmdPipelineBarrier(
       cmd_buffer->vk(), sourceStage, destinationStage, 0, 0, nullptr, 0, nullptr, 1, &barrier);

   auto graphics_bit = _device->physical_device()->get_queue_family(VK_QUEUE_GRAPHICS_BIT);
   if (!graphics_bit) {
      throw std::runtime_error("No graphics bit");
   }
   cmd_buffer->end_and_submit(_device, pool);
   _current_layout = new_layout;
}

void Image::copy_from_buffer(Buffer* buffer, CommandPool* pool)
{
   auto cmd = CommandBuffer::begin_one_time_submit(_device, pool);

   VkBufferImageCopy region{};
   region.bufferOffset = 0;
   region.bufferRowLength = 0;
   region.bufferImageHeight = 0;

   region.imageSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
   region.imageSubresource.mipLevel = 0;
   region.imageSubresource.baseArrayLayer = 0;
   region.imageSubresource.layerCount = 1;

   region.imageOffset = {.x = 0, .y = 0, .z = 0};
   region.imageExtent = {.width = static_cast<uint32_t>(_extent.width),
                         .height = static_cast<uint32_t>(_extent.height),
                         .depth = 1};

   vkCmdCopyBufferToImage(
       cmd->vk(), buffer->vk(), _image, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &region);

   auto graphics_bit = _device->physical_device()->get_queue_family(VK_QUEUE_GRAPHICS_BIT);
   if (!graphics_bit) {
      throw std::runtime_error("No graphics bit");
   }
   cmd->end_and_submit(_device, pool);
}

void Image::generate_mipmaps(CommandPool* pool)
{
   // Check if image format supports linear blitting
   VkFormatProperties formatProperties;
   vkGetPhysicalDeviceFormatProperties(
       _device->physical_device()->vk(), _config.format, &formatProperties);

   if (!(formatProperties.optimalTilingFeatures &
         VK_FORMAT_FEATURE_SAMPLED_IMAGE_FILTER_LINEAR_BIT)) {
      throw std::runtime_error("Texture image format does not support linear blitting!");
   }

   auto cmd = CommandBuffer::begin_one_time_submit(_device, pool);
   if (!cmd) {
      throw std::runtime_error(std::format("{}", cmd.error().full_message()));
   }

   // Calculate number of mip levels
   uint32_t mipLevels =
       static_cast<uint32_t>(std::floor(std::log2(std::max(_extent.width, _extent.height)))) + 1;

   VkImageMemoryBarrier barrier{};
   barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
   barrier.image = _image;
   barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
   barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
   barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
   barrier.subresourceRange.baseArrayLayer = 0;
   barrier.subresourceRange.layerCount = 1;
   barrier.subresourceRange.levelCount = 1;

   auto mipWidth = static_cast<int32_t>(_extent.width);
   auto mipHeight = static_cast<int32_t>(_extent.height);

   for (uint32_t i = 1; i < mipLevels; i++) {
      // Transition previous mip level to VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL
      barrier.subresourceRange.baseMipLevel = i - 1;
      barrier.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
      barrier.newLayout = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;
      barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
      barrier.dstAccessMask = VK_ACCESS_TRANSFER_READ_BIT;

      vkCmdPipelineBarrier(cmd->vk(),
                           VK_PIPELINE_STAGE_TRANSFER_BIT,
                           VK_PIPELINE_STAGE_TRANSFER_BIT,
                           0,
                           0,
                           nullptr,
                           0,
                           nullptr,
                           1,
                           &barrier);

      // Blit from previous level to current level
      VkImageBlit blit{};
      blit.srcOffsets[0] = {.x = 0, .y = 0, .z = 0};
      blit.srcOffsets[1] = {.x = mipWidth, .y = mipHeight, .z = 1};
      blit.srcSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
      blit.srcSubresource.mipLevel = i - 1;
      blit.srcSubresource.baseArrayLayer = 0;
      blit.srcSubresource.layerCount = 1;
      blit.dstOffsets[0] = {.x = 0, .y = 0, .z = 0};
      blit.dstOffsets[1] = {
          .x = mipWidth > 1 ? mipWidth / 2 : 1, .y = mipHeight > 1 ? mipHeight / 2 : 1, .z = 1};
      blit.dstSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
      blit.dstSubresource.mipLevel = i;
      blit.dstSubresource.baseArrayLayer = 0;
      blit.dstSubresource.layerCount = 1;

      vkCmdBlitImage(cmd->vk(),
                     _image,
                     VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
                     _image,
                     VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
                     1,
                     &blit,
                     VK_FILTER_LINEAR);

      barrier.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;
      barrier.newLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
      barrier.srcAccessMask = VK_ACCESS_TRANSFER_READ_BIT;
      barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;

      vkCmdPipelineBarrier(cmd->vk(),
                           VK_PIPELINE_STAGE_TRANSFER_BIT,
                           VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT,
                           0,
                           0,
                           nullptr,
                           0,
                           nullptr,
                           1,
                           &barrier);

      if (mipWidth > 1) mipWidth /= 2;
      if (mipHeight > 1) mipHeight /= 2;
   }

   // Transition last mip level to VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL
   barrier.subresourceRange.baseMipLevel = mipLevels - 1;
   barrier.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
   barrier.newLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
   barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
   barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;

   vkCmdPipelineBarrier(cmd->vk(),
                        VK_PIPELINE_STAGE_TRANSFER_BIT,
                        VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT,
                        0,
                        0,
                        nullptr,
                        0,
                        nullptr,
                        1,
                        &barrier);

   auto graphics_bit = _device->physical_device()->get_queue_family(VK_QUEUE_GRAPHICS_BIT);
   if (!graphics_bit) {
      throw std::runtime_error("No graphics bit");
   }
   // cmd->end_and_submit(graphics_bit.value());
   _current_layout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
}

void Image::populate_image_view()
{
   if (!_image) {
      meddl::log::error("Can not populate an image view without a valid image");
   }
   auto info = _config.get_view_create_info(_image);
   meddl::log::info("Creating image view with format: {}", static_cast<int32_t>(info.format));
   if (vkCreateImageView(_device->vk(), &info, _device->get_allocators(), &_image_view) !=
       VK_SUCCESS) {
      meddl::log::error("Create image view failed, GG");
   }
}

}  // namespace meddl::render::vk
