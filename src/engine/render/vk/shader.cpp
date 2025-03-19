#include "engine/render/vk/shader.h"

#include <expected>
#include <fstream>

#include "core/error.h"

namespace meddl::render::vk {

namespace {
std::expected<std::string, meddl::error::ErrorInfo> read_file(const std::filesystem::path& path)
{
   try {
      std::ifstream file(path, std::ios::binary);
      if (!file) {
         return std::unexpected(meddl::error::make_error(
             std::make_error_code(std::errc::io_error), "Failed to open file: {}", path.string()));
      }
      return std::string((std::istreambuf_iterator<char>(file)), std::istreambuf_iterator<char>());
   }
   catch (const std::exception& e) {
      return std::unexpected(meddl::error::make_error(
          std::make_error_code(std::errc::io_error), "Error reading file: {}", e.what()));
   }
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
   if (!source.has_value()) {
      std::println("{}", source.error().message());
      return {};
   }
   auto result = _compiler.CompileGlslToSpv(
       source.value().c_str(), source.value().size(), kind, path.c_str(), _options);

   if (result.GetCompilationStatus() != shaderc_compilation_status_success) {
      throw std::runtime_error{
          std::format("Shader compilation failed: {}", result.GetErrorMessage())};
   }
   std::vector<uint32_t> result_binary{};
   result_binary.reserve(std::distance(result.begin(), result.end()) * sizeof(uint32_t));

   std::ranges::copy(result, std::back_inserter(result_binary));
   std::println("Compiled shader size: {}", result_binary.size());

   return result_binary;
}

void ShaderProgram::load_vertex(const std::filesystem::path& path)
{
   ShaderCompiler compiler;
   _vertex = compiler.compile(path, shaderc_vertex_shader);
}

}  // namespace meddl::render::vk
