#include "vulkan_renderer/device.h"

#include <iostream>
#include <vector>

#include "wrappers/vulkan/surface_khr.h"

PhysicalDevice::PhysicalDevice(VkPhysicalDevice handle, meddl::vulkan::SurfaceKHR& surface)
    : m_handle(handle) {
   uint32_t n_families = 0;
   vkGetPhysicalDeviceQueueFamilyProperties(m_handle, &n_families, nullptr);

   std::vector<VkQueueFamilyProperties> queue_families(n_families);
   vkGetPhysicalDeviceQueueFamilyProperties(m_handle, &n_families, queue_families.data());

   for (int idx = 0; const auto& queue_family : queue_families) {
      if (queue_family.queueFlags & VK_QUEUE_GRAPHICS_BIT) {
         m_family_indicies.graphics = idx;
      }

      if (queue_family.queueFlags & VK_QUEUE_COMPUTE_BIT) {
         m_family_indicies.compute = idx;
      }

      if (queue_family.queueFlags & VK_QUEUE_TRANSFER_BIT) {
         m_family_indicies.transfer = idx;
      }

      VkBool32 has_present = false;
      // VkSurfaceKHR s = surface;
      vkGetPhysicalDeviceSurfaceSupportKHR(handle, idx, surface, &has_present);
      if (has_present) {
         m_family_indicies.present = idx;
      }

      idx++;
   }
}

PhysicalDevice::PhysicalDevice(VkPhysicalDevice handle, VkSurfaceKHR& surface) : m_handle(handle) {
   uint32_t n_families = 0;
   vkGetPhysicalDeviceQueueFamilyProperties(m_handle, &n_families, nullptr);

   std::vector<VkQueueFamilyProperties> queue_families(n_families);
   vkGetPhysicalDeviceQueueFamilyProperties(m_handle, &n_families, queue_families.data());

   for (int idx = 0; const auto& queue_family : queue_families) {
      if (queue_family.queueFlags & VK_QUEUE_GRAPHICS_BIT) {
         m_family_indicies.graphics = idx;
      }

      if (queue_family.queueFlags & VK_QUEUE_COMPUTE_BIT) {
         m_family_indicies.compute = idx;
      }

      if (queue_family.queueFlags & VK_QUEUE_TRANSFER_BIT) {
         m_family_indicies.transfer = idx;
      }

      VkBool32 has_present = false;
      vkGetPhysicalDeviceSurfaceSupportKHR(handle, idx, surface, &has_present);
      if (has_present) {
         m_family_indicies.present = idx;
      }

      idx++;
   }
}

PhysicalDevice::operator VkPhysicalDevice() const {
   return m_handle;
}

[[nodiscard]] std::optional<uint32_t> PhysicalDevice::get_graphics_index() const {
   return m_family_indicies.graphics;
}
[[nodiscard]] std::optional<uint32_t> PhysicalDevice::get_compute_index() const {
   return m_family_indicies.compute;
}
[[nodiscard]] std::optional<uint32_t> PhysicalDevice::get_present_index() const {
   return m_family_indicies.present;
}
[[nodiscard]] std::optional<uint32_t> PhysicalDevice::get_transfer_index() const {
   return m_family_indicies.transfer;
}
