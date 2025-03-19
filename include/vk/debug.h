#pragma once

#include <vulkan/vulkan_core.h>

#include <set>
#include <string>
#include <unordered_map>
#include <vector>

#include "GLFW/glfw3.h"
#include "core/log.h"

namespace meddl::vk {

enum class ValidationLayerSeverity : uint16_t {
   Error = VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT,
   Warning = Error | VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT,
   Info = Warning | VK_DEBUG_UTILS_MESSAGE_SEVERITY_INFO_BIT_EXT,
   Verbose = Info | VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT,
};

enum class ValidationLayerType : uint8_t {
   General = VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT,
   Validation = VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT,
   Performance = VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT,
   DeviceAddressBinding = VK_DEBUG_UTILS_MESSAGE_TYPE_DEVICE_ADDRESS_BINDING_BIT_EXT,
   All = General | Validation | Performance | DeviceAddressBinding
};

//! Configuration for users
struct DebugConfiguration {
   DebugConfiguration() {meddl::log::info("Building a config?");}
   std::set<std::string> layers{"VK_LAYER_KHRONOS_validation"};
   std::set<ValidationLayerType> validation_types{ValidationLayerType::All};
   ValidationLayerSeverity msg_severity{ValidationLayerSeverity::Warning};

   bool enable_debug_markers{false};
   bool enable_gpu_validation{false};
   bool enable_best_practices{false};
   bool enable_sync_validation{false};

   bool disable_shader_validation{false};
   bool disable_thread_safety{false};

   void* user_data{nullptr};
   PFN_vkDebugUtilsMessengerCallbackEXT custom_callback{nullptr};
};

//! Struct to chain all create_infos together, where the vectors
//! only live as long as necessary 
struct DebugCreateInfoChain {
   VkDebugUtilsMessengerCreateInfoEXT debug_create_info{};
   VkValidationFeaturesEXT validation_features{};
   std::vector<VkValidationFeatureEnableEXT> enabled_features{};
   std::vector<VkValidationFeatureDisableEXT> disabled_features{};
};

//! @note Debugger and instance has kinda a circular dependency
//! so this class should be owned by Instance
class Debugger {
  public:
   Debugger(DebugConfiguration config);
   ~Debugger() = default;
   Debugger(Debugger&&) = default;
   Debugger& operator=(Debugger&&) = default;
   Debugger(const Debugger&) = delete;
   Debugger& operator=(const Debugger&) = delete;

   const std::unordered_map<std::string, VkLayerProperties>& get_available_validation_layers()
       const;
   const std::set<std::string>& get_active_validation_layers() const;

   VkDebugUtilsMessengerEXT* get_messenger();
   DebugCreateInfoChain make_create_info_chain();

   void init(VkInstance instance, const DebugCreateInfoChain& chain);
   void deinit(VkInstance instance);

   // Debug object naming
   void set_object_name(VkDevice device,
                        uint64_t object_handle,
                        VkObjectType object_type,
                        const char* name);

   // Debug markers for command buffers
   void begin_region(VkCommandBuffer cmd_buffer, const char* name, const float color[4] = nullptr);
   void begin_region(VkQueue queue, const char* name, const float color[4] = nullptr);
   void end_region(VkCommandBuffer cmd_buffer);
   void end_region(VkQueue queue);
   void insert_label(VkCommandBuffer cmd_buffer, const char* name, const float color[4] = nullptr);
   void insert_label(VkQueue queue, const char* name, const float color[4] = nullptr);

  private:
   DebugConfiguration _config;
   VkDebugUtilsMessengerEXT _messenger{VK_NULL_HANDLE};
   std::unordered_map<std::string, VkLayerProperties> _available_validation_layers;
   std::set<std::string> _active_validation_layers;

   PFN_vkSetDebugUtilsObjectNameEXT vkSetDebugUtilsObjectNameEXT{nullptr};
   PFN_vkCmdBeginDebugUtilsLabelEXT vkCmdBeginDebugUtilsLabelEXT{nullptr};
   PFN_vkCmdEndDebugUtilsLabelEXT vkCmdEndDebugUtilsLabelEXT{nullptr};
   PFN_vkCmdInsertDebugUtilsLabelEXT vkCmdInsertDebugUtilsLabelEXT{nullptr};
   PFN_vkQueueBeginDebugUtilsLabelEXT vkQueueBeginDebugUtilsLabelEXT{nullptr};
   PFN_vkQueueEndDebugUtilsLabelEXT vkQueueEndDebugUtilsLabelEXT{nullptr};
   PFN_vkQueueInsertDebugUtilsLabelEXT vkQueueInsertDebugUtilsLabelEXT{nullptr};
};

}  // namespace meddl::vk
