#pragma once
#include <vulkan/vulkan_core.h>

#include "core/error.h"

namespace meddl::render::vk {

enum class ErrCode {
   None,
   // Vulkan errors
   OutOfHostMemory,
   OutOfDeviceMemory,
   DeviceLost,
   // Component-specific errors
   NoPhysicalDevices,
   InvalidDevice,
   UnsupportedFeature,
   SwapchainOutdated,
   SurfaceLost,
};

class Error : public meddl::error::Error {
  public:
   ErrCode code;
   VkResult vk_result{VK_SUCCESS};

   Error(std::string_view msg,
         ErrCode err_code,
         VkResult result = VK_SUCCESS,
         std::source_location loc = std::source_location::current())
       : meddl::error::Error(msg, loc), code(err_code), vk_result(result)
   {
   }

   // Helper for creating errors from VkResult
   static Error from_result(VkResult result, std::string_view operation)
   {
      ErrCode code{ErrCode::None};
      switch (result) {
         case VK_ERROR_OUT_OF_HOST_MEMORY:
            code = ErrCode::OutOfHostMemory;
            break;
         case VK_ERROR_OUT_OF_DEVICE_MEMORY:
            code = ErrCode::OutOfDeviceMemory;
            break;
         case VK_ERROR_DEVICE_LOST:
            code = ErrCode::DeviceLost;
            break;
         case VK_ERROR_SURFACE_LOST_KHR:
            code = ErrCode::SurfaceLost;
            break;
         case VK_ERROR_OUT_OF_DATE_KHR:
            code = ErrCode::SwapchainOutdated;
            break;
         default:
            code = ErrCode::None;
            break;
      }
      return {std::format("{} failed with result {}", operation, static_cast<int32_t>(result)),
              code,
              result};
   }

   // Helper for creating errors from code
   static Error from_code(ErrCode code, std::string_view message)
   {
      return {std::string(message), code};
   }
};
}  // namespace meddl::error::vk
