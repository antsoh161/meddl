#include "engine/render/vk/texture.h"

#include <vulkan/vulkan_core.h>

#include <cstring>

#include "engine/render/vk/buffer.h"
#include "engine/render/vk/command.h"

namespace meddl::render::vk {

std::expected<Texture, error::Error> Texture::create(Device* device, const ImageData& image_data)
{
   if (image_data.pixels.empty()) {
      return std::unexpected(error::Error("Can not create texture from empty image data"));
   }
   Texture texture;

   VkFormat format = VK_FORMAT_R8G8B8A8_SRGB;
   if (!image_data.format_hint.empty()) {
      // Map format hints to Vulkan formats
      if (image_data.format_hint == "RGBA8") {
         format = VK_FORMAT_R8G8B8A8_SRGB;
      }
      else if (image_data.format_hint == "RGB8") {
         format = VK_FORMAT_R8G8B8_SRGB;
      }
      else if (image_data.format_hint == "BC7") {
         format = VK_FORMAT_BC7_SRGB_BLOCK;
      }
   }
   else {
      switch (image_data.channels) {
         case 1:
            format = VK_FORMAT_R8_UNORM;
            break;
         case 2:
            format = VK_FORMAT_R8G8_UNORM;
            break;
         case 3:
            format = VK_FORMAT_R8G8B8_SRGB;
            break;
         case 4:
            format = VK_FORMAT_R8G8B8A8_SRGB;
            break;
         default:
            break;
      }
   }

   VkDeviceSize image_size = image_data.size_bytes();
   auto staging_buffer =
       Buffer(device,
              image_size,
              VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
              VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);

   void* data{nullptr};
   vkMapMemory(device->vk(), staging_buffer.memory(), 0, image_size, 0, &data);
   std::memcpy(data, image_data.pixels.data(), static_cast<size_t>(image_size));
   vkUnmapMemory(device->vk(), staging_buffer.memory());

   uint32_t mip_levels = 1;
   if (image_data.generate_mipmaps) {
      mip_levels = static_cast<uint32_t>(
                       std::floor(std::log2(std::max(image_data.width, image_data.height)))) +
                   1;
   }

   // Define image usage flags
   VkImageUsageFlags usage = VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT;
   if (image_data.generate_mipmaps) {
      usage |= VK_IMAGE_USAGE_TRANSFER_SRC_BIT;
   }

   Image image = Image::create_texture(
       device, image_data.width, image_data.height, format, mip_levels, usage);

   auto graphics_bit = device->physical_device()->get_queue_family(VK_QUEUE_GRAPHICS_BIT);
   if (!graphics_bit) {
      return std::unexpected(error::Error("Can not create texture from without a graphics bit"));
   }
   auto pool =
       CommandPool::create(device, graphics_bit.value(), VK_COMMAND_POOL_CREATE_TRANSIENT_BIT);
   if (!pool) {
      return std::unexpected(error::Error("Pool creation failed in texture creation"));
   }
   if (image_data.generate_mipmaps) {
      meddl::log::debug("Transition from UNDEFINED to DST OPTIMAL");
      image.transition(
          &pool.value(), VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL);
      image.copy_from_buffer(&staging_buffer, &pool.value());

      meddl::log::debug("Generating mipmaps...");
      image.generate_mipmaps(&pool.value());
      meddl::log::debug("Done generating mipmaps...");
   }
   else {
      meddl::log::debug("No mipmaps, regular shader read optimal");
      // First transition to transfer dst optimal
      image.transition(
          &pool.value(), VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL);
      // Then copy data
      image.copy_from_buffer(&staging_buffer, &pool.value());
      // Then transition to shader read only
      image.transition(&pool.value(),
                       VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
                       VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);
   }

   Sampler::Cfg sampler_cfg{};
   sampler_cfg.magFilter = VK_FILTER_LINEAR;
   sampler_cfg.minFilter = VK_FILTER_LINEAR;
   sampler_cfg.mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR;
   sampler_cfg.addressModeU = VK_SAMPLER_ADDRESS_MODE_REPEAT;
   sampler_cfg.addressModeV = VK_SAMPLER_ADDRESS_MODE_REPEAT;
   sampler_cfg.addressModeW = VK_SAMPLER_ADDRESS_MODE_REPEAT;
   sampler_cfg.anisotropyEnable = VK_TRUE;
   sampler_cfg.maxAnisotropy =
       device->physical_device()->get_properties().limits.maxSamplerAnisotropy;
   sampler_cfg.compareEnable = VK_FALSE;
   sampler_cfg.minLod = 0.0f;
   sampler_cfg.maxLod = static_cast<float>(mip_levels);

   auto sampler = Sampler::create(device, sampler_cfg);
   if (!sampler) {
      return std::unexpected(error::Error("Sampler creation failed in texture creation"));
   }
   return texture;
}

}  // namespace meddl::render::vk
