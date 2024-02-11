#include "vulkan_renderer/device.h"

#include <iostream>
#include <vector>

namespace meddl::vulkan {

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
      _available_extensions.insert(ext.extensionName);
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

// bool PhysicalDevice::fulfills_requirement(const PhysicalDeviceQueueProperties& pdr) const {
//    for (const auto& family : _queue_families) {
//       if (family._capabilities.find(pdr) == family._capabilities.end()) {
//          return false;
//       }
//    }
//    return true;
// }
// [[nodiscard]] bool PhysicalDevice::fulfills_requirement(
//     const std::set<std::string>& requested_extensions) const {
//
//    return true;
// }

[[nodiscard]] std::vector<QueueFamily>& PhysicalDevice::get_queue_families()
{
   return _queue_families;
}
//! ------------------------------------------------------
//! Logical device

LogicalDevice::operator VkDevice() const
{
   return _active_logical_device;
}
LogicalDevice::operator VkDevice*()
{
   return &_active_logical_device;
}

}  // namespace meddl::vulkan
