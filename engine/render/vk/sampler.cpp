#include "engine/render/vk/sampler.h"

#include "engine/render/vk/device.h"

namespace meddl::render::vk {

Sampler::Sampler(Device* device) : _device(device) {}

Sampler::~Sampler()
{
   if (_sampler && _device) {
      vkDestroySampler(_device->vk(), _sampler, nullptr);
   }
}

Sampler::Sampler(Sampler&& other) noexcept
    : _config(std::move(other._config)), _device(other._device), _sampler(other._sampler)
{
   other._device = nullptr;
   other._sampler = VK_NULL_HANDLE;
}

Sampler& Sampler::operator=(Sampler&& other) noexcept
{
   if (this != &other) {
      _device = other._device;
      _sampler = other._sampler;
      _config = std::move(other._config);
      other._device = nullptr;
      other._sampler = VK_NULL_HANDLE;
   }
   return *this;
}

std::expected<Sampler, error::Error> Sampler::create(Device* device, const Cfg& createInfo)
{
   Sampler sampler(device);
   sampler._config = createInfo;

   VkSamplerCreateInfo samplerInfo{};
   samplerInfo.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
   samplerInfo.magFilter = createInfo.magFilter;
   samplerInfo.minFilter = createInfo.minFilter;
   samplerInfo.addressModeU = createInfo.addressModeU;
   samplerInfo.addressModeV = createInfo.addressModeV;
   samplerInfo.addressModeW = createInfo.addressModeW;
   samplerInfo.anisotropyEnable = createInfo.anisotropyEnable;
   samplerInfo.maxAnisotropy = createInfo.maxAnisotropy;
   samplerInfo.borderColor = createInfo.borderColor;
   samplerInfo.unnormalizedCoordinates = VK_FALSE;
   samplerInfo.compareEnable = createInfo.compareEnable;
   samplerInfo.compareOp = createInfo.compareOp;
   samplerInfo.mipmapMode = createInfo.mipmapMode;
   samplerInfo.mipLodBias = createInfo.mipLodBias;
   samplerInfo.minLod = createInfo.minLod;
   samplerInfo.maxLod = createInfo.maxLod;

   VkResult result = vkCreateSampler(device->vk(), &samplerInfo, nullptr, &sampler._sampler);
   if (result != VK_SUCCESS) {
      return std::unexpected(
          error::Error(std::format("vkCreateSampler failed: {}", static_cast<int32_t>(result))));
   }

   return sampler;
}

}  // namespace meddl::render::vk
