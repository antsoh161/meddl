#include "vulkan_renderer/device.h"

#include <iostream>
#include <vector>

namespace meddl::vulkan {

PhysicalDevice::PhysicalDevice(VkPhysicalDevice handle, VkSurfaceKHR& surface) : _handle(handle) {
   uint32_t n_families = 0;
   vkGetPhysicalDeviceQueueFamilyProperties(_handle, &n_families, nullptr);

   std::vector<VkQueueFamilyProperties> queue_families(n_families);
   vkGetPhysicalDeviceQueueFamilyProperties(_handle, &n_families, queue_families.data());
   for (int idx = 0; const auto& queue_family : queue_families) {
      if (queue_family.queueFlags & VK_QUEUE_GRAPHICS_BIT) {
         _pdr_to_index_map.emplace(PhysicalDeviceRequirements::PD_GRAPHICS, idx);
         std::cout << "GRAPHICS FOUND!\n";
      }

      if (queue_family.queueFlags & VK_QUEUE_COMPUTE_BIT) {
         _pdr_to_index_map.emplace(PhysicalDeviceRequirements::PD_COMPUTE, idx);
         std::cout << "COMPUTE FOUND!\n";
      }

      if (queue_family.queueFlags & VK_QUEUE_TRANSFER_BIT) {
         _pdr_to_index_map.emplace(PhysicalDeviceRequirements::PD_TRANSFER, idx);
         std::cout << "TRANSFER FOUND!\n";
      }

      VkBool32 has_present = false;
      vkGetPhysicalDeviceSurfaceSupportKHR(handle, idx, surface, &has_present);
      if (has_present) {
         std::cout << "PRESENT FOUND!\n";
         _pdr_to_index_map.emplace(PhysicalDeviceRequirements::PD_PRESENT, idx);
      }

      idx++;
   }
}

PhysicalDevice::operator VkPhysicalDevice() const {
   return _handle;
}

bool PhysicalDevice::fulfills_requirement(const std::set<PhysicalDeviceRequirements>& pdr) const {
   for (const auto& req : pdr) {
      if (_pdr_to_index_map.find(req) == _pdr_to_index_map.end()) {
         return false;
      }
   }
   return true;
}
bool PhysicalDevice::fulfills_requirement(const PhysicalDeviceRequirements& pdr) const {
   return _pdr_to_index_map.find(pdr) == _pdr_to_index_map.end() ? false : true;
}

[[nodiscard]] const std::unordered_map<PhysicalDeviceRequirements, uint32_t>&
PhysicalDevice::get_pdr_to_index_map() const {
   return _pdr_to_index_map;
}

//! ------------------------------------------------------
//! Logical device
LogicalDevice::LogicalDevice(VkDevice handle, QueueFamilies queue_families)
    : _active_logical_device(handle), _queues(queue_families) {
}

LogicalDevice::operator VkDevice() const {
   return _active_logical_device;
}

[[nodiscard]] VkQueue& LogicalDevice::get_queue(const PhysicalDeviceRequirements& pdr) {
   switch(pdr){
      case PhysicalDeviceRequirements::PD_GRAPHICS:
         return _queues._graphics_queue;
      case PhysicalDeviceRequirements::PD_PRESENT:
         return _queues._present_queue;
      case PhysicalDeviceRequirements::PD_COMPUTE:
         return _queues._compute_queue;
      case PhysicalDeviceRequirements::PD_TRANSFER:
         return _queues._transfer_queue;
   }
}
}  // namespace meddl::vulkan
