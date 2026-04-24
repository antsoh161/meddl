#include "engine/shader.h"

#include <expected>
#include <fstream>
#include <shaderc/shaderc.hpp>

#include "core/error.h"
#include "engine/loader.h"

namespace meddl::engine::loader {

namespace {
std::expected<std::string, ShaderError> read_file(const std::filesystem::path& path)
{
   try {
      std::ifstream file(path, std::ios::binary);
      if (!file) {
         return std::unexpected(
             ShaderError::from_code(ShaderError::Code::FileNotFound,
                                    std::format("Shader file not found: {}", path.string())));
      }
      return std::string((std::istreambuf_iterator<char>(file)), std::istreambuf_iterator<char>());
   }
   catch (const std::exception& e) {
      return std::unexpected(
          ShaderError::from_code(ShaderError::Code::FileException,
                                 std::format("Shader file caught exception: {}", e.what())));
   }
}
}  // namespace

shaderc_shader_kind shader_kind_from_path(const std::filesystem::path& path)
{
   const auto ext = path.extension().string();

   if (ext == ".vert") return shaderc_glsl_vertex_shader;
   if (ext == ".frag") return shaderc_glsl_fragment_shader;
   if (ext == ".comp") return shaderc_glsl_compute_shader;
   if (ext == ".geom") return shaderc_glsl_geometry_shader;
   if (ext == ".tesc") return shaderc_glsl_tess_control_shader;
   if (ext == ".tese") return shaderc_glsl_tess_evaluation_shader;
   if (ext == ".rgen") return shaderc_glsl_raygen_shader;
   if (ext == ".rint") return shaderc_glsl_intersection_shader;
   if (ext == ".rahit") return shaderc_glsl_anyhit_shader;
   if (ext == ".rchit") return shaderc_glsl_closesthit_shader;
   if (ext == ".rmiss") return shaderc_glsl_miss_shader;
   if (ext == ".rcall") return shaderc_glsl_callable_shader;
   if (ext == ".mesh") return shaderc_glsl_mesh_shader;
   if (ext == ".task") return shaderc_glsl_task_shader;

   // Default or error case
   return shaderc_glsl_infer_from_source;
}

std::expected<ShaderData, ShaderError> compile_glsl(const std::string& source,
                                                    shaderc_shader_kind kind,
                                                    const std::string& filename,
                                                    const std::string& entry_point)
{
   ShaderData result;
   result.entry_point = entry_point;

   // Set shader type string for debugging
   switch (kind) {
      case shaderc_glsl_vertex_shader:
         result.shader_type = "vertex";
         break;
      case shaderc_glsl_fragment_shader:
         result.shader_type = "fragment";
         break;
      case shaderc_glsl_compute_shader:
         result.shader_type = "compute";
         break;
      default:
         result.shader_type = "other";
         break;
   }

   // Setup compiler and options
   shaderc::Compiler compiler;
   shaderc::CompileOptions options;

   // Set optimization level
   options.SetOptimizationLevel(shaderc_optimization_level_performance);

   // Compile to SPIR-V
   auto compilation_result = compiler.CompileGlslToSpv(
       source.c_str(), source.size(), kind, filename.c_str(), entry_point.c_str(), options);

   if (compilation_result.GetCompilationStatus() != shaderc_compilation_status_success) {
      return std::unexpected(ShaderError::from_code(
          ShaderError::Code::CompilationFailed,
          std::format("Shader compilation failed: {}", compilation_result.GetErrorMessage())));
   }

   // Copy the compiled SPIR-V binary
   result.spirv_code = {compilation_result.begin(), compilation_result.end()};

   return result;
}

std::expected<ShaderData, ShaderError> compile_shader_file(const std::filesystem::path& path,
                                                           const std::string& entry_point)
{
   auto source_result = read_file(path);
   if (!source_result) {
      return std::unexpected(source_result.error());
   }

   return compile_glsl(
       source_result.value(), shader_kind_from_path(path), path.filename().string(), entry_point);
}

std::expected<ShaderData, ShaderError> load_shader(const std::filesystem::path& path,
                                                   const std::string& entry_point)
{
   return compile_shader_file(path, entry_point);
}

}  // namespace meddl::engine::loader
