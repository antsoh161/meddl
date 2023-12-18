#pragma once

#include <vulkan/vulkan_core.h>

#include <unordered_map>
#include <vector>
#include <string>

VkDebugUtilsMessengerCreateInfoEXT default_info();

class VulkanDebugger {
 private:
  VkDebugUtilsMessengerEXT m_messenger;
  // Vulkan wants const char*
  std::unordered_map<std::string, VkLayerProperties> m_available_validation_layers;
  std::vector<std::string> m_active_validation_layers;

 public:
  VulkanDebugger();
  void verify_validation_layers();
  const std::unordered_map<std::string, VkLayerProperties>& get_available_validation_layers() const;
  const std::vector<std::string>& get_active_validation_layers() const;
  void add_validation_layer(const std::string& layer);
  void add_validation_layers(const std::vector<std::string>& layers);

  VkDebugUtilsMessengerEXT* get_messenger();
  void build();
};
