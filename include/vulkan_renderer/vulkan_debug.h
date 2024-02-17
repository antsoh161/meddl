#pragma once

#include <string>
#include <unordered_map>
#include <vector>

#include "GLFW/glfw3.h"

class VulkanDebugger {
  public:
   VulkanDebugger();
   ~VulkanDebugger() = default;
   VulkanDebugger(VulkanDebugger&&) = default;
   VulkanDebugger& operator=(VulkanDebugger&&) = default;
   VulkanDebugger(const VulkanDebugger&) = delete;
   VulkanDebugger& operator=(const VulkanDebugger&) = delete;

   void verify_validation_layers();
   const std::unordered_map<std::string, VkLayerProperties>& get_available_validation_layers()
       const;
   const std::vector<std::string>& get_active_validation_layers() const;
   void add_validation_layer(const std::string& layer);
   void add_validation_layers(const std::vector<std::string>& layers);

   VkDebugUtilsMessengerEXT* get_messenger();
   void clean_up(VkInstance instance);

  private:
   VkDebugUtilsMessengerEXT _messenger;
   // Vulkan wants const char*
   std::unordered_map<std::string, VkLayerProperties> _available_validation_layers;
   // TODO: Make this a set
   std::vector<std::string> _active_validation_layers;
};
