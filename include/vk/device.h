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

  private:
   VkDevice _active_logical_device{VK_NULL_HANDLE};
   std::unordered_map<uint32_t, VkQueue> _logical_queues{};
};

struct QueueSettings {
   float _priority{defaults::DEFAULT_QUEUE_PRIORITY};
   uint32_t _queue_count{defaults::DEFAULT_QUEUE_COUNT};
};

class NewPhysicalDevice {
  public:
   NewPhysicalDevice(Instance* instance, VkPhysicalDevice device);

   operator VkPhysicalDevice() const { return _device; }
   [[nodiscard]] VkPhysicalDevice vk() const { return _device; }

   Instance* instance() { return _instance; }

   [[nodiscard]] bool fulfills_requirement(const std::set<PhysicalDeviceQueueProperties>& pdr,
                                           const std::set<std::string>& requested_extensions) const;

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

class DeviceExtensions {};
// Surface,
class NewDevice {
  public:
   NewDevice() = delete;
   NewDevice(NewPhysicalDevice* physical_device,
             const std::unordered_map<uint32_t, QueueConfiguration>& queue_configurations,
             const std::unordered_set<std::string>& device_extensions,
             const std::optional<VkPhysicalDeviceFeatures>& device_features = std::nullopt,
             const std::optional<Debugger>& debugger = std::nullopt);
   ~NewDevice();

   NewDevice(const NewDevice&) = delete;
   NewDevice& operator=(const NewDevice&) = delete;
   NewDevice(NewDevice&&) = default;
   NewDevice& operator=(NewDevice&&) = default;

   operator VkDevice() const { return _device; }
   [[nodiscard]] VkDevice vk() const { return _device; }

  private:
   VkDevice _device{};
   Instance* _instance;
   NewPhysicalDevice* _physical_device;
   std::vector<DeviceExtensions> _extensions{};
   std::vector<Queue> _queues{};
};
}  // namespace meddl::vk
