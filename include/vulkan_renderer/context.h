#pragma once

#include <vulkan/vulkan_core.h>

#include <optional>
#include <set>
#include <unordered_set>
#include <vector>

#include "GLFW/glfw3.h"
#include "vulkan_renderer/device.h"
#include "vulkan_renderer/swapchain.h"
#include "vulkan_renderer/vulkan_debug.h"
#include "wrappers/glfw/window.h"

namespace meddl::vulkan {

class Context {
  public:
   Context(std::shared_ptr<glfw::Window>&& window,
           std::optional<VulkanDebugger>&& debugger,
           VkDebugUtilsMessengerCreateInfoEXT debug_info,
           VkApplicationInfo app_info,
           const std::set<PhysicalDeviceQueueProperties>& pdr,
           const std::set<std::string>& device_extensions);

   Context(const Context&) = delete;
   Context& operator=(const Context&) = delete;

   // TODO: this ok?
   Context(Context&& other) noexcept = default;
   Context& operator=(Context&& other) noexcept = default;

   ~Context();

  private:
   void clean_up();
   VkResult make_instance(const VkApplicationInfo& app_info,
                          const VkDebugUtilsMessengerCreateInfoEXT& debug_info);
   bool make_physical_devices();
   bool pick_suitable_device(const std::set<PhysicalDeviceQueueProperties>& pdr,
                             const std::set<std::string>& requested_extensions);
   bool make_logical_device(const std::set<std::string>& device_extensions);
   // bool make_swapchain();

   // TODO: Should a vulkan context hold this?
   std::shared_ptr<glfw::Window> _window;
   VkInstance _instance{VK_NULL_HANDLE};
   VkSurfaceKHR _surface{VK_NULL_HANDLE};
   std::vector<PhysicalDevice> _available_devices{};
   PhysicalDevice* _active_device{nullptr};  // This just points to an element in the device vector
   LogicalDevice _logical_device;
   SwapChain _swapchain;
   std::optional<VulkanDebugger> _debugger{};
};

// TODO: Move to meddl::vulkan::debug ?
namespace {
VKAPI_ATTR VkBool32 VKAPI_CALL
meddl_debug_callback(VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity,
                     VkDebugUtilsMessageTypeFlagsEXT messageType,
                     const VkDebugUtilsMessengerCallbackDataEXT* pCallbackData,
                     void* pUserData) {
   std::cerr << "validation layer: " << pCallbackData->pMessage << std::endl;

   return VK_FALSE;
}
}  // namespace

class ContextBuilder {
  public:
   ContextBuilder& window(std::shared_ptr<glfw::Window> window);
   ContextBuilder& enable_debugger();
   ContextBuilder& with_debug_layers(const std::vector<std::string>& layers);
   ContextBuilder& with_debug_create_info(const VkDebugUtilsMessengerCreateInfoEXT& debug_info);
   ContextBuilder& with_app_info(const VkApplicationInfo& app_info);
   ContextBuilder& with_physical_device_requirements(const PhysicalDeviceQueueProperties& pdr);
   ContextBuilder& with_physical_device_requirements(
       const std::set<PhysicalDeviceQueueProperties>& pdr);
   ContextBuilder& with_required_device_extensions(
       const std::set<std::string>& device_extensions);
   Context build();

  private:
   // TODO: Will this ptr be thread safe?
   std::shared_ptr<glfw::Window> _window;
   VkApplicationInfo _app_info{VK_STRUCTURE_TYPE_APPLICATION_INFO,
                               nullptr,
                               "Default Meddl Application",
                               VK_MAKE_VERSION(1, 0, 0),
                               "No engine",
                               VK_MAKE_VERSION(1, 0, 0),
                               VK_API_VERSION_1_0};
   VkDebugUtilsMessengerCreateInfoEXT _debug_info{
       VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT,
       nullptr,
       0,
       VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT |
           VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT |
           VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT,
       VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT |
           VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT |
           VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT,
       meddl_debug_callback,
       nullptr,
   };
   std::set<PhysicalDeviceQueueProperties> _pdr{PhysicalDeviceQueueProperties::PD_GRAPHICS,
                                                PhysicalDeviceQueueProperties::PD_PRESENT};
   std::set<std::string> _device_extensions{};
   std::vector<std::string> _debug_layers{};

   std::optional<VulkanDebugger> _debugger;
};
}  // namespace meddl::vulkan
