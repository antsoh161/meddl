#include "vulkan_renderer/vulkan_debug.h"

#include <iostream>
#include <stdexcept>

namespace {
VKAPI_ATTR VkBool32 VKAPI_CALL
debug_callback(VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity,
               VkDebugUtilsMessageTypeFlagsEXT messageType,
               const VkDebugUtilsMessengerCallbackDataEXT* pCallbackData, void* pUserData) {
  std::cerr << "validation layer: " << pCallbackData->pMessage << std::endl;

  return VK_FALSE;
}
}  // namespace

VkDebugUtilsMessengerCreateInfoEXT default_info() {
  VkDebugUtilsMessengerCreateInfoEXT info{};
  info.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT;
  info.messageSeverity = VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT |
                         VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT |
                         VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT;
  info.messageType = VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT |
                     VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT |
                     VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT;
  info.pfnUserCallback = debug_callback;
  return info;
}

VulkanDebugger::VulkanDebugger() {
  uint32_t n_layers{};
  vkEnumerateInstanceLayerProperties(&n_layers, nullptr);

  std::vector<VkLayerProperties> layers{};
  layers.resize(n_layers);
  vkEnumerateInstanceLayerProperties(&n_layers, layers.data());

  for (const auto& layer : layers) {
    m_available_validation_layers.emplace(layer.layerName, layer);
  }
}

void VulkanDebugger::add_validation_layer(const std::string& layer) {
  auto found = m_available_validation_layers.find(layer);
  if (found != m_available_validation_layers.end()) {
  } else {
    throw std::runtime_error("Validation layer not available: " + std::string(layer));
  }
}

void VulkanDebugger::add_validation_layers(const std::vector<std::string>& layers) {
  for (const auto& layer : layers) {
    auto found = m_available_validation_layers.find(layer);
    if (found != m_available_validation_layers.end()) {
      std::cout << "pushing layer: " << layer << "\n";
      m_active_validation_layers.push_back(layer);
    } else {
      throw std::runtime_error("Validation layer not available: " + std::string(layer));
    }
  }
}

const std::unordered_map<std::string, VkLayerProperties>&
VulkanDebugger::get_available_validation_layers() const {
  return m_available_validation_layers;
}

const std::vector<std::string>& VulkanDebugger::get_active_validation_layers() const {
  return m_active_validation_layers;
}

VkDebugUtilsMessengerEXT* VulkanDebugger::get_messenger() {
  return &m_messenger;
}
