#pragma once

#include <vulkan/vulkan_core.h>

#include "engine/render/vk/image.h"

namespace meddl::render::vk {

class Image;
class Device;

class Texture {
  public:
   Texture(Device* device, const std::string& filepath);
   ~Texture();

   Texture(const Texture&) = delete;
   Texture& operator=(const Texture&) = delete;
   Texture(Texture&&) noexcept;
   Texture& operator=(Texture&&) noexcept;

   // [[nodiscard]] VkImageView view() const { return _image.view(); }
   [[nodiscard]] VkSampler sampler() const { return _sampler; }

  private:
   Device* _device;
   std::optional<Image> _image{std::nullopt};
   VkSampler _sampler = VK_NULL_HANDLE;
};

}  // namespace meddl::render::vk
