#pragma once

#include <cstdint>
#include <set>
#include <string>
#include <unordered_map>
#include <vector>

#include "GLFW/glfw3.h"
namespace meddl::vk {

enum class PhysicalDeviceQueueProperties {
   PD_GRAPHICS,
   PD_PRESENT,
   PD_COMPUTE,
   PD_TRANSFER,
};

struct QueueFamily {
   uint32_t _idx{};
   std::set<PhysicalDeviceQueueProperties> _capabilities{};
   VkQueue _queue{};
};

//! Physical device handler holder, with device capabilities
class PhysicalDevice {
  public:
   PhysicalDevice(VkPhysicalDevice handle, VkSurfaceKHR& surface);

   operator VkPhysicalDevice() const;

   [[nodiscard]] bool fulfills_requirement(const std::set<PhysicalDeviceQueueProperties>& pdr,
                                           const std::set<std::string>& requested_extensions) const;
   [[nodiscard]] std::vector<QueueFamily>& get_queue_families();

  private:
   VkPhysicalDevice _handle{VK_NULL_HANDLE};
   std::vector<QueueFamily> _queue_families{};
   std::set<std::string> _available_extensions;
};

//! Logical device representation, with the family queues
class LogicalDevice {
  public:
   LogicalDevice() = default;

   operator VkDevice();
   operator VkDevice*();

   // [[nodiscard]] VkQueue& get_queue(const PhysicalDeviceQueueProperties& pdr);

  private:
   VkDevice _active_logical_device{VK_NULL_HANDLE};
   std::unordered_map<uint32_t, VkQueue> _logical_queues{};
};
}  // namespace meddl::vk
