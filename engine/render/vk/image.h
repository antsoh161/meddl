#pragma once

#include <vulkan/vulkan_core.h>

#include <optional>

#include "engine/render/vk/device.h"
#include "engine/render/vk/shared.h"
namespace meddl::render::vk {

class CommandBuffer;
class CommandPool;
class Buffer;
class Image {
  public:
   Image() = default;
   ~Image();
   Image(const Image&) = delete;
   Image& operator=(const Image&) = delete;
   Image(Image&& other) noexcept;
   Image& operator=(Image&& other) noexcept;

   //! Resources owned by this
   static Image create(Device* device,
                       const GraphicsConfiguration::AttachmentConfig& config,
                       uint32_t width,
                       uint32_t height);

   //! This image does not own the handle or the memory
   static Image create_deferred(VkImage image,
                                Device* device,
                                const GraphicsConfiguration::AttachmentConfig& config);

   static Image create_texture(Device* device,
                               uint32_t width,
                               uint32_t height,
                               VkFormat format,
                               uint32_t mip_levels,
                               VkImageUsageFlags usage);

   [[nodiscard]] VkImage vk() const { return _image; }
   [[nodiscard]] VkImageView view() const { return _image_view; }
   [[nodiscard]] VkDeviceMemory memory() const { return _memory; }
   [[nodiscard]] bool is_owner() const { return _owned_resources.has_value(); }

   void transition(CommandPool* pool, VkImageLayout old_layout, VkImageLayout new_layout);
   void copy_from_buffer(Buffer* buffer, CommandPool* pool);
   void generate_mipmaps(CommandPool* pool);

   enum class ImageType { Owned, Deferred, Texture };

  private:
   Image(Device* device, const GraphicsConfiguration::AttachmentConfig& config);
   void populate_image_view();

   struct Owned {
      VkDeviceMemory memory;
   };
   Device* _device{VK_NULL_HANDLE};
   GraphicsConfiguration::AttachmentConfig _config{};

   VkImage _image{VK_NULL_HANDLE};
   VkImageView _image_view{VK_NULL_HANDLE};
   VkImageLayout _current_layout{VK_IMAGE_LAYOUT_UNDEFINED};
   VkDeviceMemory _memory{VK_NULL_HANDLE};
   VkExtent3D _extent{};
   ImageType _type{ImageType::Owned};
   std::optional<Owned> _owned_resources{std::nullopt};
};
}  // namespace meddl::render::vk
