#include "engine/render/vk/debug.h"

#include <vulkan/vulkan_core.h>

#include <algorithm>
#include <numeric>
#include <stdexcept>

#include "core/log.h"
#include "engine/render/vk/command.h"
#include "engine/render/vk/device.h"
#include "engine/render/vk/queue.h"

namespace {
constexpr VKAPI_ATTR VkBool32 VKAPI_CALL
default_callback(VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity,
                 VkDebugUtilsMessageTypeFlagsEXT messageType,
                 const VkDebugUtilsMessengerCallbackDataEXT* pCallbackData,
                 void* pUserData)
{
   (void)pUserData;
   std::string type_str;
   if (messageType & VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT) {
      type_str += "GENERAL";
   }
   if (messageType & VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT) {
      type_str += type_str.empty() ? "VALIDATION" : "|VALIDATION";
   }
   if (messageType & VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT) {
      type_str += type_str.empty() ? "PERFORMANCE" : "|PERFORMANCE";
   }

   if (messageSeverity & VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT) {
      meddl::log::error("Vk ERROR [{}]: {}", type_str.c_str(), pCallbackData->pMessage);
   }
   else if (messageSeverity & VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT) {
      meddl::log::warn("Vk WARNING[{}]: {}", type_str.c_str(), pCallbackData->pMessage);
   }
   else if (messageSeverity & VK_DEBUG_UTILS_MESSAGE_SEVERITY_INFO_BIT_EXT) {
      meddl::log::info("Vk INFO[{}]: {}", type_str.c_str(), pCallbackData->pMessage);
   }
   else {
      meddl::log::debug("Vk GENERAL [{}]: {}", type_str.c_str(), pCallbackData->pMessage);
   }
   return VK_FALSE;
}
}  // namespace

namespace meddl::render::vk {

Debugger::Debugger(DebugConfiguration config) : _config(config)
{
   uint32_t n_layers{};
   vkEnumerateInstanceLayerProperties(&n_layers, nullptr);

   std::vector<VkLayerProperties> layers{};
   layers.resize(n_layers);
   vkEnumerateInstanceLayerProperties(&n_layers, layers.data());
   for (const auto& layer : layers) {
      _available_validation_layers.emplace(layer.layerName, layer);
   }
   for (const auto& requested_layer : config.layers) {
      if (_available_validation_layers.find(requested_layer) !=
          _available_validation_layers.end()) {
         _active_validation_layers.insert(requested_layer);
      }
   }
}

void Debugger::set_object_name(Device* device,
                               uint64_t object_handle,
                               VkObjectType type,
                               const std::string& name)
{
   if (!_config.enable_debug_markers || !vkSetDebugUtilsObjectNameEXT) {
      return;
   }
   VkDebugUtilsObjectNameInfoEXT name_info{};
   name_info.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_OBJECT_NAME_INFO_EXT;
   name_info.objectType = type;
   name_info.objectHandle = object_handle;
   name_info.pObjectName = name.c_str();

   vkSetDebugUtilsObjectNameEXT(device->vk(), &name_info);
}

void Debugger::begin_region(Queue* queue,
                            const std::string& name,
                            const std::array<float, 4>& color)
{
   if (!_config.enable_debug_markers || !vkQueueBeginDebugUtilsLabelEXT) {
      return;
   }

   VkDebugUtilsLabelEXT info{};
   info.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_LABEL_EXT;
   info.pLabelName = name.c_str();
   std::ranges::copy(color, std::begin(info.color));
   vkQueueBeginDebugUtilsLabelEXT(queue->vk(), &info);
}

void Debugger::begin_region(CommandBuffer* buffer,
                            const std::string& name,
                            const std::array<float, 4>& color)
{
   if (!_config.enable_debug_markers || !vkCmdBeginDebugUtilsLabelEXT) {
      return;
   }
   if (buffer->state() != CommandBuffer::State::Recording) {
      meddl::log::error("Can not debug a buffer region if its not recording");
   }

   VkDebugUtilsLabelEXT info{};
   info.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_LABEL_EXT;
   info.pLabelName = name.c_str();
   std::ranges::copy(color, std::begin(info.color));
   vkCmdBeginDebugUtilsLabelEXT(buffer->vk(), &info);
}

void Debugger::end_region(Queue* queue)
{
   if (!_config.enable_debug_markers || !vkQueueEndDebugUtilsLabelEXT) {
      return;
   }
   vkQueueEndDebugUtilsLabelEXT(queue->vk());
}

void Debugger::end_region(CommandBuffer* buffer)
{
   if (!_config.enable_debug_markers || !vkCmdEndDebugUtilsLabelEXT) {
      return;
   }
   vkCmdEndDebugUtilsLabelEXT(buffer->vk());
}

void Debugger::insert_label(Queue* queue,
                            const std::string& name,
                            const std::array<float, 4>& color)
{
   if (!_config.enable_debug_markers || !vkQueueInsertDebugUtilsLabelEXT) {
      return;
   }
   VkDebugUtilsLabelEXT info{};
   info.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_LABEL_EXT;
   info.pLabelName = name.c_str();
   std::ranges::copy(color, std::begin(info.color));
   vkQueueInsertDebugUtilsLabelEXT(queue->vk(), &info);
}

void Debugger::insert_label(CommandBuffer* buffer,
                            const std::string& name,
                            const std::array<float, 4>& color)
{
   if (!_config.enable_debug_markers || !vkQueueInsertDebugUtilsLabelEXT) {
      return;
   }
   VkDebugUtilsLabelEXT info{};
   info.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_LABEL_EXT;
   info.pLabelName = name.c_str();
   std::ranges::copy(color, std::begin(info.color));
   vkCmdInsertDebugUtilsLabelEXT(buffer->vk(), &info);
}

void Debugger::init(VkInstance instance, const DebugCreateInfoChain& chain)
{
   if (_active_validation_layers.empty()) {
      return;
   }

   auto func = std::bit_cast<PFN_vkCreateDebugUtilsMessengerEXT>(
       vkGetInstanceProcAddr(instance, "vkCreateDebugUtilsMessengerEXT"));

   if (!func) {
      throw std::runtime_error("Failed to load vkCreateDebugUtilsMessengerEXT function");
   }

   if (func(instance, &chain.debug_create_info, nullptr, &_messenger) != VK_SUCCESS) {
      throw std::runtime_error("Failed to set up debug messenger");
   }

   // Load debug marker function pointers if debug markers are enabled
   if (_config.enable_debug_markers) {
      vkSetDebugUtilsObjectNameEXT = std::bit_cast<PFN_vkSetDebugUtilsObjectNameEXT>(
          vkGetInstanceProcAddr(instance, "vkSetDebugUtilsObjectNameEXT"));
      vkCmdBeginDebugUtilsLabelEXT = std::bit_cast<PFN_vkCmdBeginDebugUtilsLabelEXT>(
          vkGetInstanceProcAddr(instance, "vkCmdBeginDebugUtilsLabelEXT"));
      vkCmdEndDebugUtilsLabelEXT = std::bit_cast<PFN_vkCmdEndDebugUtilsLabelEXT>(
          vkGetInstanceProcAddr(instance, "vkCmdEndDebugUtilsLabelEXT"));
      vkCmdInsertDebugUtilsLabelEXT = std::bit_cast<PFN_vkCmdInsertDebugUtilsLabelEXT>(
          vkGetInstanceProcAddr(instance, "vkCmdInsertDebugUtilsLabelEXT"));
      vkQueueBeginDebugUtilsLabelEXT = std::bit_cast<PFN_vkQueueBeginDebugUtilsLabelEXT>(
          vkGetInstanceProcAddr(instance, "vkQueueBeginDebugUtilsLabelEXT"));
      vkQueueEndDebugUtilsLabelEXT = std::bit_cast<PFN_vkQueueEndDebugUtilsLabelEXT>(
          vkGetInstanceProcAddr(instance, "vkQueueEndDebugUtilsLabelEXT"));
      vkQueueInsertDebugUtilsLabelEXT = std::bit_cast<PFN_vkQueueInsertDebugUtilsLabelEXT>(
          vkGetInstanceProcAddr(instance, "vkQueueInsertDebugUtilsLabelEXT"));
   }
}

void Debugger::deinit(VkInstance instance)
{
   auto func = std::bit_cast<PFN_vkDestroyDebugUtilsMessengerEXT>(
       vkGetInstanceProcAddr(instance, "vkDestroyDebugUtilsMessengerEXT"));
   if (func) {
      func(instance, _messenger, nullptr /* Allocator */);
   }
}

DebugCreateInfoChain Debugger::make_create_info_chain()
{
   DebugCreateInfoChain chain;

   const auto message_type =
       std::accumulate(_config.validation_types.begin(),
                       _config.validation_types.end(),
                       0U,
                       [](auto acc, auto type) {
                          return acc | static_cast<VkDebugUtilsMessageTypeFlagsEXT>(type);
                       });

   chain.debug_create_info.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT;
   chain.debug_create_info.pNext = nullptr;
   chain.debug_create_info.flags = 0;  // None available, as far as I can tell
   chain.debug_create_info.messageSeverity =
       static_cast<VkDebugUtilsMessageSeverityFlagsEXT>(_config.msg_severity);
   chain.debug_create_info.messageType = message_type;
   chain.debug_create_info.pfnUserCallback =
       _config.custom_callback ? _config.custom_callback : default_callback;
   chain.debug_create_info.pUserData = _config.user_data;

   if (_config.enable_gpu_validation) {
      chain.enabled_features.push_back(VK_VALIDATION_FEATURE_ENABLE_GPU_ASSISTED_EXT);
      chain.enabled_features.push_back(
          VK_VALIDATION_FEATURE_ENABLE_GPU_ASSISTED_RESERVE_BINDING_SLOT_EXT);
   }

   if (_config.enable_best_practices) {
      chain.enabled_features.push_back(VK_VALIDATION_FEATURE_ENABLE_BEST_PRACTICES_EXT);
   }

   if (_config.enable_sync_validation) {
      chain.enabled_features.push_back(VK_VALIDATION_FEATURE_ENABLE_SYNCHRONIZATION_VALIDATION_EXT);
   }

   if (_config.disable_shader_validation) {
      chain.disabled_features.push_back(VK_VALIDATION_FEATURE_DISABLE_SHADERS_EXT);
   }

   if (_config.disable_thread_safety) {
      chain.disabled_features.push_back(VK_VALIDATION_FEATURE_DISABLE_THREAD_SAFETY_EXT);
   }

   if (!chain.enabled_features.empty() || !chain.disabled_features.empty()) {
      chain.validation_features.sType = VK_STRUCTURE_TYPE_VALIDATION_FEATURES_EXT;
      chain.validation_features.pNext = nullptr;

      if (!chain.enabled_features.empty()) {
         chain.validation_features.enabledValidationFeatureCount =
             static_cast<uint32_t>(chain.enabled_features.size());
         chain.validation_features.pEnabledValidationFeatures = chain.enabled_features.data();
      }

      if (!chain.disabled_features.empty()) {
         chain.validation_features.disabledValidationFeatureCount =
             static_cast<uint32_t>(chain.disabled_features.size());
         chain.validation_features.pDisabledValidationFeatures = chain.disabled_features.data();
      }

      chain.debug_create_info.pNext = &chain.validation_features;
   }

   return chain;
}

const std::unordered_map<std::string, VkLayerProperties>&
Debugger::get_available_validation_layers() const
{
   return _available_validation_layers;
}

const std::set<std::string>& Debugger::get_active_validation_layers() const
{
   return _active_validation_layers;
}

VkDebugUtilsMessengerEXT* Debugger::get_messenger()
{
   return &_messenger;
}

}  // namespace meddl::render::vk
