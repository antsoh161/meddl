#include "vk/device.h"

#include <iostream>
#include <vector>

#include "vk/instance.h"
#include "vk/surface.h"

namespace meddl::vk {

PhysicalDevice::PhysicalDevice(VkPhysicalDevice handle, VkSurfaceKHR& surface) : _handle(handle)
{
   // Queue family capabilities
   uint32_t n_families = 0;
   vkGetPhysicalDeviceQueueFamilyProperties(_handle, &n_families, nullptr);

   std::vector<VkQueueFamilyProperties> queue_families(n_families);
   vkGetPhysicalDeviceQueueFamilyProperties(_handle, &n_families, queue_families.data());
   for (int idx = 0; const auto& queue_family : queue_families) {
      QueueFamily qf;
      qf._idx = idx;
      if (queue_family.queueFlags & VK_QUEUE_GRAPHICS_BIT) {
         qf._capabilities.insert(PhysicalDeviceQueueProperties::PD_GRAPHICS);
      }
      if (queue_family.queueFlags & VK_QUEUE_COMPUTE_BIT) {
      }

      if (queue_family.queueFlags & VK_QUEUE_TRANSFER_BIT) {
         qf._capabilities.insert(PhysicalDeviceQueueProperties::PD_TRANSFER);
      }
      VkBool32 has_present = false;
      vkGetPhysicalDeviceSurfaceSupportKHR(handle, idx, surface, &has_present);
      if (has_present) {
         qf._capabilities.insert(PhysicalDeviceQueueProperties::PD_PRESENT);
      }
      _queue_families.push_back(qf);
      idx++;
   }

   // Device extension capabilities
   uint32_t ext_count{};
   vkEnumerateDeviceExtensionProperties(_handle, nullptr, &ext_count, nullptr);
   std::vector<VkExtensionProperties> available_extensions(ext_count);
   vkEnumerateDeviceExtensionProperties(_handle, nullptr, &ext_count, available_extensions.data());
   for (auto& ext : available_extensions) {
      _available_extensions.insert(ext.extensionName);  // NOLINT
   }
}

PhysicalDevice::operator VkPhysicalDevice() const
{
   return _handle;
}

bool PhysicalDevice::fulfills_requirement(const std::set<PhysicalDeviceQueueProperties>& pdr,
                                          const std::set<std::string>& requested_extensions) const
{
   for (const auto& req : pdr) {
      for (const auto& family : _queue_families) {
         if (family._capabilities.find(req) == family._capabilities.end()) {
            return false;
         }
      }
   }
   for (const auto& req : requested_extensions) {
      if (_available_extensions.find(req) == _available_extensions.end()) {
         return false;
      }
   }
   std::cout << "All requirements fulfilled!\n";
   return true;
}

[[nodiscard]] std::vector<QueueFamily>& PhysicalDevice::get_queue_families()
{
   return _queue_families;
}
//! ------------------------------------------------------
//! Logical device

LogicalDevice::operator VkDevice()
{
   return _active_logical_device;
}
LogicalDevice::operator VkDevice*()
{
   return &_active_logical_device;
}

//! Physical Device
//!

NewPhysicalDevice::NewPhysicalDevice(Instance* instance, VkPhysicalDevice device)
    : _device(device), _instance(instance)
{
   vkGetPhysicalDeviceFeatures(_device, &_features);

   vkGetPhysicalDeviceProperties(_device, &_properties);

   uint32_t n_families = 0;
   vkGetPhysicalDeviceQueueFamilyProperties(_device, &n_families, nullptr);

   _queue_families.resize(n_families);
   vkGetPhysicalDeviceQueueFamilyProperties(_device, &n_families, _queue_families.data());

   // instance->getProcAddr(_vkGetPhysicalDeviceFeatures2,
   // "vkGetPhysicalDeviceFeatures2", "vkGetPhysicalDeviceFeatures2KHR");
   // instance->getProcAddr(_vkGetPhysicalDeviceProperties2,
   // "vkGetPhysicalDeviceProperties2", "vkGetPhysicalDeviceProperties2KHR");
}

std::vector<VkQueueFamilyProperties>& NewPhysicalDevice::get_queue_families()
{
   return _queue_families;
}

std::optional<int> NewPhysicalDevice::get_queue_family(VkQueueFlags flags)
{
   for (int idx = 0; const auto& queue_family : _queue_families) {
      if ((queue_family.queueFlags & flags) == flags) {
         return idx;
      }
      idx++;
   }
   return {};
}

std::optional<int> NewPhysicalDevice::get_present_family(Surface* surface)
{
   VkBool32 has_present{false};
   for (int i = 0; i < _queue_families.size(); i++) {
      vkGetPhysicalDeviceSurfaceSupportKHR(_device, i, *surface, &has_present);
      if (has_present) return i;
   }
   return {};
}

const VkPhysicalDeviceFeatures& NewPhysicalDevice::get_features() const
{
   return _features;
}

//! Device
NewDevice::NewDevice(NewPhysicalDevice* physical_device,
                     const std::unordered_map<uint32_t, QueueConfiguration>& queue_configurations,
                     const std::unordered_set<std::string>& device_extensions,
                     const std::optional<VkPhysicalDeviceFeatures>& device_features,
                     const std::optional<Debugger>& debugger)
    : _instance(physical_device->instance()), _physical_device((physical_device))
{
   std::vector<VkDeviceQueueCreateInfo> create_infos{};

   auto make_cinfo = [](uint32_t queue_family_index,
                        const QueueConfiguration& configuration) -> VkDeviceQueueCreateInfo {
      VkDeviceQueueCreateInfo queue_cinfo{};
      queue_cinfo.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
      queue_cinfo.queueFamilyIndex = queue_family_index;
      queue_cinfo.queueCount = configuration._queue_count;
      queue_cinfo.pQueuePriorities = &configuration._priority;
      return queue_cinfo;
   };
   for (const auto& config : queue_configurations) {
      create_infos.push_back(make_cinfo(config.first, config.second));
   }

   VkDeviceCreateInfo create_info{};
   create_info.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
   create_info.queueCreateInfoCount = create_infos.size();
   create_info.pQueueCreateInfos = create_infos.data();
   create_info.pEnabledFeatures = device_features.has_value() ? &device_features.value() : nullptr;
   create_info.enabledExtensionCount = static_cast<uint32_t>(device_extensions.size());

   std::vector<const char*> extensions_cstyle{};
   for (const auto& ext : device_extensions) {
      extensions_cstyle.push_back(ext.c_str());
   }
   create_info.ppEnabledExtensionNames = extensions_cstyle.data();

   std::vector<const char*> layers_cstyle{};
   if (debugger.has_value()) {
      create_info.enabledLayerCount = debugger->get_active_validation_layers().size();
      const auto& layers = debugger->get_active_validation_layers();
      for (const auto& layer : layers) {
         layers_cstyle.push_back(layer.c_str());
      }
      create_info.ppEnabledLayerNames = layers_cstyle.data();
   }
   else {
      create_info.enabledLayerCount = 0;
   }

   auto res = vkCreateDevice(*_physical_device, &create_info, nullptr, &_device);
   if (res != VK_SUCCESS) {
      throw std::runtime_error{
          std::format("vkCreateDevice failed with error: {}", static_cast<int32_t>(res))};
   }

   for (auto& config : queue_configurations) {
      for (uint32_t i = 0; i < config.second._queue_count; i++) {
         VkQueue queue{};
         vkGetDeviceQueue(_device, config.first, i, &queue);
         _queues.emplace_back(queue, i, config.second);
      }
   }
}

NewDevice::~NewDevice()
{
   if (_device) {
      // TODO: Allocator
      vkDestroyDevice(_device, nullptr);
   }
}
}  // namespace meddl::vk
