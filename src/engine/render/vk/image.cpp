#include "engine/render/vk/image.h"

#include <stdexcept>

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
   auto image_info = config.get_image_create_info(width, height);
   if (vkCreateImage(device->vk(), &image_info, device->get_allocators(), &result._image) !=
       VK_SUCCESS) {
      meddl::log::error("Failed to create image, GG");
   }

   // Get memory requirements
   VkMemoryRequirements mem_requirements;
   vkGetImageMemoryRequirements(device->vk(), result._image, &mem_requirements);
   uint32_t type_index{0};
   bool found_suitable = false;

   auto mem_properties = device->physical_device()->get_memory_properties();
   for (uint32_t i = 0; i < mem_properties.memoryTypeCount; i++) {
      if ((mem_requirements.memoryTypeBits & (1 << i)) &&
          (mem_properties.memoryTypes[i].propertyFlags & config.memory_flags) ==
              config.memory_flags) {
         type_index = i;
         found_suitable = true;
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
