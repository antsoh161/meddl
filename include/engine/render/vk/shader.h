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

class ShaderCompiler {
  public:
   ShaderCompiler() = default;

   [[nodiscard]] shaderc::CompileOptions& options() { return _options; }

   std::vector<uint32_t> compile(const std::filesystem::path& path,
                                 const shaderc_shader_kind& kind);

  private:
   shaderc::Compiler _compiler{};
   shaderc::CompileOptions _options{};
};

class ShaderProgram {
  public:
   ShaderProgram() = default;
   void load_vertex(const std::filesystem::path& path);
   bool load_fragment(const std::filesystem::path& path);

  private:
   // SPIRV's
   std::vector<uint32_t> _vertex{};
   std::vector<uint32_t> _fragment{};
};
}  // namespace meddl::render::vk
