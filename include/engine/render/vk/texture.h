#pragma once

#include <vulkan/vulkan_core.h>

#include "engine/render/vk/image.h"
#include "engine/render/vk/sampler.h"
#include "engine/types.h"

namespace meddl::render::vk {

class Image;
class Device;
class Sampler;

class Texture {
  public:
   enum class Type {
      Standard2D,
      CubeMap,
      Array2D,
      Volume,
   };
   Texture() = default;
   static std::expected<Texture, error::Error> create(Device* device, const ImageData& image_data);

   ~Texture() = default;
   Texture(const Texture&) = delete;
   Texture& operator=(const Texture&) = delete;
   Texture(Texture&& other) noexcept = default;
   Texture& operator=(Texture&& other) noexcept = default;

   [[nodiscard]] const Image& image() const { return _image; }
   [[nodiscard]] Image& image() { return _image; }
   [[nodiscard]] const Sampler& sampler() const { return _sampler; }
   [[nodiscard]] Sampler& sampler() { return _sampler; }

  private:
   Image _image;
   Sampler _sampler;
   Type _type{Type::Standard2D};
};

}  // namespace meddl::render::vk
