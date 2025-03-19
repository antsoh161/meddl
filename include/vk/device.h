#pragma once

#include <cstdint>
#include <set>
#include <string>
#include <unordered_map>
#include <unordered_set>
#include <vector>

#include "GLFW/glfw3.h"
#include "vk/defaults.h"
#include "vk/instance.h"
#include "vk/queue.h"

namespace meddl::vk {
class Surface;
class Instance;

struct QueueSettings {
   float _priority{defaults::DEFAULT_QUEUE_PRIORITY};
   uint32_t _queue_count{defaults::DEFAULT_QUEUE_COUNT};
};

class PhysicalDevice {
  public:
   PhysicalDevice(Instance* instance, VkPhysicalDevice device);

   operator VkPhysicalDevice() const { return _device; }
   [[nodiscard]] VkPhysicalDevice vk() const { return _device; }

   Instance* instance() { return _instance; }

   //! @brief
   //! Returns the queue family only if it's a perfect match to flags
   [[nodiscard]] std::optional<int> get_queue_family(VkQueueFlags flags);
   [[nodiscard]] std::optional<int> get_present_family(Surface* surface);
   [[nodiscard]] const VkPhysicalDeviceFeatures& get_features() const;
   [[nodiscard]] std::vector<VkQueueFamilyProperties>& get_queue_families();

  private:
   VkPhysicalDevice _device;
   Instance* _instance;

   VkPhysicalDeviceFeatures _features{};
   VkPhysicalDeviceProperties _properties{};
   std::vector<VkQueueFamilyProperties> _queue_families{};
   PFN_vkGetPhysicalDeviceFeatures2 _vkGetPhysicalDeviceFeatures2 = nullptr;
   PFN_vkGetPhysicalDeviceProperties2 _vkGetPhysicalDeviceProperties2 = nullptr;
};

// TODO: Device extensions
class DeviceExtensions {};

class Device {
  public:
   Device() = delete;
   Device(PhysicalDevice* physical_device,
          const std::unordered_map<uint32_t, QueueConfiguration>& queue_configurations,
          const std::unordered_set<std::string>& device_extensions,
          const std::optional<VkPhysicalDeviceFeatures>& device_features = std::nullopt,
          const std::set<std::string>& validation_layers = {});
   ~Device();

   Device(const Device&) = delete;
   Device& operator=(const Device&) = delete;
   Device(Device&&) = default;
   Device& operator=(Device&&) = default;

   operator VkDevice() const { return _device; }
   [[nodiscard]] VkDevice vk() const { return _device; }

   const std::vector<Queue>& queues() { return _queues; }
   PhysicalDevice* physical_device() { return _physical_device; }

   void wait_idle();
   // TODO: Allocator
   VkAllocationCallbacks* get_allocators() { return nullptr; }

  private:
   std::vector<Queue> _queues{};
   VkDevice _device{};
   Instance* _instance;
   PhysicalDevice* _physical_device;
   std::vector<DeviceExtensions> _extensions{};
};
}  // namespace meddl::vk
