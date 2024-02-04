#pragma once

#include <vulkan/vulkan_core.h>

#include <cstdint>
#include <optional>
#include <set>
#include <unordered_map>

#include "GLFW/glfw3.h"
#include "vulkan_renderer/vulkan_debug.h"
namespace meddl::vulkan {

enum class PhysicalDeviceRequirements {
   PD_GRAPHICS,
   PD_PRESENT,
   PD_COMPUTE,
   PD_TRANSFER,
};

struct QueueFamilies {
   VkQueue _graphics_queue{};
   VkQueue _present_queue{};
   VkQueue _compute_queue{};
   VkQueue _transfer_queue{};
};

//! Physical device handler holder, with device capabilities
class PhysicalDevice {
  public:
   PhysicalDevice(VkPhysicalDevice handle, VkSurfaceKHR& surface);

   operator VkPhysicalDevice() const;

   bool fulfills_requirement(const std::set<PhysicalDeviceRequirements>& pdr) const;
   bool fulfills_requirement(const PhysicalDeviceRequirements& pdr) const;
   [[nodiscard]] const std::unordered_map<PhysicalDeviceRequirements, uint32_t>&
   get_pdr_to_index_map() const;

  private:
   VkPhysicalDevice _handle{VK_NULL_HANDLE};
   std::unordered_map<PhysicalDeviceRequirements, uint32_t> _pdr_to_index_map{};
};

//! Logical device representation, with the family queues
class LogicalDevice {
  public:
   LogicalDevice() = default;
   LogicalDevice(VkDevice _handle, QueueFamilies queue_families);

   operator VkDevice() const ;

   [[nodiscard]] VkQueue& get_queue(const PhysicalDeviceRequirements& pdr);

  private:
   VkDevice _active_logical_device{VK_NULL_HANDLE};
   QueueFamilies _queues;
};
}  // namespace meddl::vulkan
