#pragma once

#include <vulkan/vulkan_core.h>

#include <expected>
#include <memory>
#include <optional>

#include "GLFW/glfw3.h"
#include "core/error.h"
#include "engine/render/vk/debug.h"
#include "engine/render/vk/defaults.h"
#include "engine/render/vk/device.h"
#include "engine/render/vk/physical_device.h"

namespace meddl::render::vk {

struct InstanceError : public meddl::error::Error {
   enum class Code {
      None,
      // vk error
      OutOfMemory,
      HostOutOfMemory,
      DeviceLost,
      // user error
      NoPhysicalDevices,
   } code;
   VkResult vk_result{VK_SUCCESS};
   InstanceError(std::string_view msg,
                 Code err_code,
                 VkResult result = VK_SUCCESS,
                 std::source_location loc = std::source_location::current())
       : Error(msg, loc), code(err_code), vk_result(result)
   {
   }
   static InstanceError from_result(VkResult result, std::string_view operation)
   {
      Code code{};
      switch (result) {
         case VK_ERROR_OUT_OF_HOST_MEMORY:
            code = Code::HostOutOfMemory;
            break;
         case VK_ERROR_OUT_OF_DEVICE_MEMORY:
            code = Code::OutOfMemory;
            break;
         case VK_ERROR_DEVICE_LOST:
            code = Code::DeviceLost;
            break;
         default:
            code = Code::None;
            break;
      }

      std::string message =
          std::format("{} failed with {}", operation, static_cast<int32_t>(result));
      return {message, code, result};
   }
   static InstanceError from_code(Code code, std::string_view operation)
   {
      std::string message = std::format("{} failed", operation);
      return {message, code};
   }
};

struct InstanceConfiguration {
   InstanceConfiguration()
   {
      // Default for now
      uint32_t glfw_ext_count = 0;
      auto glfw_ext = glfwGetRequiredInstanceExtensions(&glfw_ext_count);
      auto ext_span = std::span(glfw_ext, glfw_ext_count);
      extensions = std::vector<std::string>(ext_span.begin(), ext_span.end());
   }
   std::string app_name{"Meddl App"};
   uint32_t app_version = VK_MAKE_VERSION(1, 0, 0);
   std::string engine_name{"Default Meddl"};
   uint32_t engine_version = VK_MAKE_VERSION(1, 0, 0);
   uint32_t api_version = VK_API_VERSION_1_3;

   //! Platform specific
   std::vector<std::string> extensions;
   bool enable_portability{false};
   //! @note override for class instance specific only
   VkAllocationCallbacks* override_allocator{nullptr};
};

class PhysicalDevice;
class Instance {
  public:
   Instance() = default;
   std::expected<Instance, InstanceError> static create(
       const InstanceConfiguration& config,
       const std::optional<DebugConfiguration>& debug_config = std::nullopt);
   ~Instance();
   Instance(const Instance&) = delete;
   Instance& operator=(const Instance&) = delete;

   Instance(Instance&&) noexcept;
   Instance& operator=(Instance&&) noexcept;

   [[nodiscard]] operator VkInstance() const { return _instance; }
   [[nodiscard]] VkInstance vk() const { return _instance; }

   [[nodiscard]] std::vector<PhysicalDevice>& get_physical_devices();
   [[nodiscard]] std::optional<Debugger>& debugger() { return _debugger; }

   void log_device_info(Surface* surface,
                        const PhysicalDeviceRequirements& reqs = {},
                        bool with_extensions = false);

  private:

   VkInstance _instance{VK_NULL_HANDLE};
   std::optional<Debugger> _debugger;
   std::vector<PhysicalDevice> _physical_devices{};
};
}  // namespace meddl::render::vk
