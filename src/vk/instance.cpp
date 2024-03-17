#include "vk/instance.h"

namespace meddl::vk {

Instance::Instance(VkApplicationInfo app_info,
                   VkDebugUtilsMessengerCreateInfoEXT debug_info,
                   std::optional<Debugger>& debugger)
{
   VkInstanceCreateInfo create_info{};
   create_info.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
   create_info.pApplicationInfo = &app_info;

   uint32_t glfw_ext_count = 0;
   auto glfw_ext = glfwGetRequiredInstanceExtensions(&glfw_ext_count);
   auto ext_span = std::span(glfw_ext, glfw_ext_count);
   std::vector<const char*> extensions(ext_span.begin(), ext_span.end());

   std::vector<const char*> layers_cstyle{};
   if (debugger.has_value()) {
      extensions.push_back(VK_EXT_DEBUG_UTILS_EXTENSION_NAME);
      create_info.enabledLayerCount = debugger->get_active_validation_layers().size();

      const auto& layers = debugger->get_active_validation_layers();
      for (const auto& layer : layers) {
         layers_cstyle.push_back(layer.c_str());
      }
      create_info.ppEnabledLayerNames = layers_cstyle.data();
      create_info.pNext = &debug_info;
   }
   else {
      create_info.enabledLayerCount = 0;
      create_info.pNext = nullptr;
   }
   create_info.enabledExtensionCount = extensions.size();
   create_info.ppEnabledExtensionNames = extensions.data();

   if (vkCreateInstance(&create_info, nullptr, &_instance) != VK_SUCCESS) {
      throw std::runtime_error("vkCreateInstance failed");
   }

   uint32_t n_devices = 0;
   vkEnumeratePhysicalDevices(_instance, &n_devices, nullptr);
   if (n_devices < 1) {
      throw std::runtime_error("No physical vulkan devices found");
   }

   std::vector<VkPhysicalDevice> devices(n_devices);
   vkEnumeratePhysicalDevices(_instance, &n_devices, devices.data());

   for (const auto& device : devices) {
      _physical_devices.emplace_back(std::make_shared<PhysicalDevice>(this, device));
   }
   // TODO: extension function pointers?
}

Instance::~Instance()
{
   _physical_devices.clear();
   vkDestroyInstance(_instance, nullptr);
}

const std::vector<std::shared_ptr<PhysicalDevice>>& Instance::get_physical_devices() const
{
   return _physical_devices;
}
}  // namespace meddl::vk
