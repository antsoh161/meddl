#include "engine/render/vk/texture.h"

#include <stdexcept>

#include "engine/render/vk/buffer.h"
#include "stb_image.h"

namespace meddl::render::vk {

Texture::Texture(Device* device, const std::string& filepath) : _device(device)
{
   // _image = Image::create(_device, {}, 0, 0);
}

Texture::~Texture()
{
   if (_sampler) {
      vkDestroySampler(_device->vk(), _sampler, nullptr);
   }
}

Texture::Texture(Texture&& other) noexcept
    : _device(other._device), _image(std::move(other._image)), _sampler(other._sampler)
{
   other._sampler = VK_NULL_HANDLE;
}

Texture& Texture::operator=(Texture&& other) noexcept
{
   if (this != &other) {
      _device = other._device;
      _image = std::move(other._image);
      _sampler = other._sampler;

      other._sampler = VK_NULL_HANDLE;
   }
   return *this;
}

}  // namespace meddl::render::vk
