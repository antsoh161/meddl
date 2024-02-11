#include "vulkan_renderer/vulkan_debug.h"

#include <iostream>
#include <stdexcept>

VulkanDebugger::VulkanDebugger() {
   uint32_t n_layers{};
   vkEnumerateInstanceLayerProperties(&n_layers, nullptr);

   std::vector<VkLayerProperties> layers{};
   layers.resize(n_layers);
   vkEnumerateInstanceLayerProperties(&n_layers, layers.data());

   for (const auto& layer : layers) {
      _available_validation_layers.emplace(layer.layerName, layer);
   }
}

void VulkanDebugger::add_validation_layer(const std::string& layer) {
   auto found = _available_validation_layers.find(layer);
   if (found != _available_validation_layers.end()) {
      _active_validation_layers.push_back(layer);
   } else {
      throw std::runtime_error("Validation layer not available: " + std::string(layer));
   }
}

void VulkanDebugger::add_validation_layers(const std::vector<std::string>& layers) {
   for (const auto& layer : layers) {
      auto found = _available_validation_layers.find(layer);
      if (found != _available_validation_layers.end()) {
         _active_validation_layers.push_back(layer);
      } else {
         throw std::runtime_error("Validation layer not available: " + std::string(layer));
      }
   }
}

const std::unordered_map<std::string, VkLayerProperties>&
VulkanDebugger::get_available_validation_layers() const {
   return _available_validation_layers;
}

const std::vector<std::string>& VulkanDebugger::get_active_validation_layers() const {
   return _active_validation_layers;
}

VkDebugUtilsMessengerEXT* VulkanDebugger::get_messenger() {
   return &_messenger;
}

void VulkanDebugger::clean_up(VkInstance instance){
    auto func = (PFN_vkDestroyDebugUtilsMessengerEXT) vkGetInstanceProcAddr(instance, "vkDestroyDebugUtilsMessengerEXT");
    if (func != nullptr) {
        func(instance, _messenger, nullptr);
    }
}
