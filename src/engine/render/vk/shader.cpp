#include "engine/render/vk/shader.h"

#include <expected>

namespace meddl::render::vk {

ShaderModule::ShaderModule(Device* device, const std::vector<uint32_t>& code) : _device(device)
{
   VkShaderModuleCreateInfo create_info = {};
   create_info.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
   create_info.codeSize = code.size() * sizeof(uint32_t);
   create_info.pCode = code.data();
   create_info.pNext = nullptr;

   auto res =
       vkCreateShaderModule(*_device, &create_info, _device->get_allocators(), &_shader_module);
   if (res != VK_SUCCESS) {
      throw std::runtime_error{
          std::format("Failed to create shader, error: {}", static_cast<int32_t>(res))};
   }
}

ShaderModule::~ShaderModule()
{
   if (_shader_module) {
      vkDestroyShaderModule(*_device, _shader_module, _device->get_allocators());
   }
}

}  // namespace meddl::render::vk
