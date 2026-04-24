#pragma once

#include <vulkan/vulkan_core.h>

#include <expected>
#include <filesystem>

#include "core/error.h"
#include "engine/loader.h"
#if defined(MEDDL_USE_SHADERC)
#include <shaderc/shaderc.hpp>
#endif

namespace meddl::engine::loader {
struct ShaderError : public meddl::error::Error {
   enum class Code {
      None,
      FileNotFound,
      FileException,
      CompilationFailed,
      InvalidShaderType,
      PreprocessorError
   } code;
   VkResult vk_result{VK_SUCCESS};
   ShaderError(std::string_view msg,
               Code err_code,
               VkResult result = VK_SUCCESS,
               std::source_location loc = std::source_location::current())
       : Error(msg, loc), code(err_code), vk_result(result)
   {
   }
   static ShaderError from_code(Code code, std::string_view operation)
   {
      std::string message = std::format("{} failed", operation);
      return {message, code};
   }
};

std::expected<ShaderData, ShaderError> load_shader(const std::filesystem::path& path,
                                                   const std::string& entry_point = "main");

std::expected<ShaderData, ShaderError> compile_glsl(const std::string& source,
                                                    shaderc_shader_kind kind,
                                                    const std::string& filename = "shader.glsl",
                                                    const std::string& entry_point = "main");

// Compile shader file to SPIR-V based on extension
std::expected<ShaderData, ShaderError> compile_shader_file(const std::filesystem::path& path,
                                                           const std::string& entry_point = "main");

// Utility to detect shader kind from file extension
shaderc_shader_kind shader_kind_from_path(const std::filesystem::path& path);

}  // namespace meddl::engine::loader
