#include "vk/shader.h"

#include <fstream>
namespace meddl::vk {

namespace {
std::optional<std::string> read_file(const std::filesystem::path& path)
{
   std::println("Reading from path: {}", path.string());
   std::ifstream file(path);
   if (file.is_open()) {
      std::println("Reading file...");
      return std::string(std::istreambuf_iterator<char>(file), std::istreambuf_iterator<char>());
   }
   return std::nullopt;
}
}  // namespace

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

std::vector<uint32_t> ShaderCompiler::compile(const std::filesystem::path& path,
                                              const shaderc_shader_kind& kind)
{
   if (!_compiler.IsValid()) {
      throw std::runtime_error("Shaderc compiler is invalid");
   }

   auto source = read_file(path);
   if (source.has_value()) {
      auto result = _compiler.CompileGlslToSpv(
          source.value().c_str(), source.value().size(), kind, path.c_str(), _options);

      if (result.GetCompilationStatus() != shaderc_compilation_status_success) {
         throw std::runtime_error{
             std::format("Shader compilation failed: {}", result.GetErrorMessage())};
      }
      std::vector<uint32_t> result_binary{};
      result_binary.reserve(std::distance(result.begin(), result.end()) * sizeof(uint32_t));

      std::transform(
          result.begin(), result.end(), std::back_inserter(result_binary), [](uint32_t bytes) {
             return bytes;
          });

      return result_binary;
   }
   // TODO: logger warn
   std::println("Compiled empty shader");
   return {};
}

}  // namespace meddl::vk
