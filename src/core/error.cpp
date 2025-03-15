#include "core/error.h"
namespace meddl::error {

std::string VulkanErrorCategory::message(int ev) const
{
   switch (static_cast<VulkanError>(ev)) {
      case VulkanError::CommandBufferNotReady:
         return "CommandBuffer not ready";
      case VulkanError::CommandBufferNotRecording:
         return "CommandBuffer not recording";
      case VulkanError::CommandBufferNotExecutable:
         return "CommandBuffer not executable";
      case VulkanError::ShaderCompilationFailed:
         return "Shader compilation failed";
      case VulkanError::ShaderFileReadError:
         return "Shader file read error";
      case VulkanError::ShaderInvalidCompiler:
         return "Shader invalid compiler";
      default:
         return "Unknown error";
   }
}

}  // namespace meddl::error
