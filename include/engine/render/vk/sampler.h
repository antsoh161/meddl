#pragma once
#include <vulkan/vulkan_core.h>

#include <expected>
#include <string>

#include "core/error.h"

namespace meddl::render::vk {
class Device;
class Sampler {
  public:
   Sampler() = default;
   ~Sampler();
   Sampler(const Sampler&) = delete;
   Sampler& operator=(const Sampler&) = delete;
   Sampler(Sampler&& other) noexcept;
   Sampler& operator=(Sampler&& other) noexcept;

   // TODO: Config.h
   struct Cfg {
      VkFilter magFilter = VK_FILTER_LINEAR;
      VkFilter minFilter = VK_FILTER_LINEAR;
      VkSamplerAddressMode addressModeU = VK_SAMPLER_ADDRESS_MODE_REPEAT;
      VkSamplerAddressMode addressModeV = VK_SAMPLER_ADDRESS_MODE_REPEAT;
      VkSamplerAddressMode addressModeW = VK_SAMPLER_ADDRESS_MODE_REPEAT;
      VkBool32 anisotropyEnable = VK_TRUE;
      float maxAnisotropy = 16.0f;
      VkBool32 compareEnable = VK_FALSE;
      VkCompareOp compareOp = VK_COMPARE_OP_ALWAYS;
      VkSamplerMipmapMode mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR;
      float mipLodBias = 0.0f;
      float minLod = 0.0f;
      float maxLod = VK_LOD_CLAMP_NONE;
      VkBorderColor borderColor = VK_BORDER_COLOR_INT_OPAQUE_BLACK;
   };

   static std::expected<Sampler, error::Error> create(Device* device, const Cfg& createInfo);

   [[nodiscard]] VkSampler vk() const { return _sampler; }

  private:
   Sampler(Device* device);
   Cfg _config{};
   Device* _device{VK_NULL_HANDLE};
   VkSampler _sampler{VK_NULL_HANDLE};
};
}  // namespace meddl::render::vk
