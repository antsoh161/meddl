#pragma once

#include <cstdint>
#include <filesystem>
#include <shaderc/shaderc.hpp>
#include <vector>

#include "engine/render/vk/device.h"

namespace meddl::render::vk {

class ShaderModule {
  public:
   ShaderModule(Device* device, const std::vector<uint32_t>& code);
   virtual ~ShaderModule();

   ShaderModule& operator=(const ShaderModule&) = delete;
   ShaderModule(const ShaderModule&) = delete;
   ShaderModule& operator=(ShaderModule&&) = default;
   ShaderModule(ShaderModule&&) = default;

   [[nodiscard]] VkShaderModule vk() const { return _shader_module; }

  private:
   Device* _device;
   VkShaderModule _shader_module{};
};

}  // namespace meddl::render::vk
