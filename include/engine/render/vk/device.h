#pragma once

#include <cstdint>
#include <expected>
#include <set>
#include <string>
#include <unordered_map>
#include <unordered_set>

#include "GLFW/glfw3.h"
#include "core/error.h"
#include "engine/render/vk/debug.h"
#include "engine/render/vk/physical_device.h"
#include "engine/render/vk/queue.h"

namespace meddl::render::vk {
// Fwd
class Surface;
class Instance;

enum class QueuePreset {
   Minimal,     // one queue per required family (minimal overhead)
   Balanced,    // separate queues for graphics/compute/transfer if available
   Performance  // max parallelism, utilize all available queues
};

struct DeviceConfiguration {
   std::unordered_map<uint32_t, QueueConfiguration> queue_configurations{};
   std::unordered_set<std::string> extensions{"VK_KHR_swapchain"};
   std::optional<VkPhysicalDeviceFeatures> features{};
   PhysicalDeviceRequirements physical_device_requirements{};
   struct {
      bool use_dedicated_allocations{true};
      VkDeviceSize buffer_image_granularity{1};
   } memory_settings;
   struct {
      bool enable_device_groups{false};
      bool enable_peer_memory{false};
   } multi_device_features;
   std::vector<std::pair<VkStructureType, void*>> feature_chain{};
   VkAllocationCallbacks* custom_allocator{nullptr};
   bool auto_enable_supported_features{false};
   bool strict_extension_requirements{true};
   QueuePreset queue_preset{QueuePreset::Balanced};
};

class Device {
  public:
   Device() = default;
   static std::expected<Device, meddl::error::Error> create(
       PhysicalDevice* physical_device,
       const DeviceConfiguration& config,
       const std::optional<Debugger>& debugger = std::nullopt);

   ~Device();

   Device(const Device&) = delete;
   Device& operator=(const Device&) = delete;
   Device(Device&&) noexcept;
   Device& operator=(Device&&) noexcept;

   operator VkDevice() const { return _device; }
   [[nodiscard]] VkDevice vk() const { return _device; }

   const std::vector<Queue>& queues() { return _queues; }
   PhysicalDevice* physical_device() { return _physical_device; }

   void wait_idle();
   // TODO: Allocator
   VkAllocationCallbacks* get_allocators() { return nullptr; }

  private:
   Device(PhysicalDevice* physical_device,
          const DeviceConfiguration& config,
          const std::optional<Debugger>& debugger);

   std::vector<Queue> _queues{};
   VkDevice _device{nullptr};
   PhysicalDevice* _physical_device{nullptr};
   std::unordered_set<std::string> _enabled_extensions{};
   VkPhysicalDeviceFeatures _enabled_features{};
};

enum class DevicePickerStrategy : uint16_t {
   HighPerformance,
   PowerEfficient,
   ComputeFocused,
};

struct DevicePickerResult {
   PhysicalDevice* best_Device{nullptr};
   DeviceConfiguration config;
};

class DevicePicker {
  public:
   DevicePicker(Instance* instance, Surface* surface);
   std::optional<DevicePickerResult> pick_best(DevicePickerStrategy strategy,
                                               bool allow_best_effort = true);
   std::optional<DevicePickerResult> pick_custom(PhysicalDeviceRequirements reqs,
                                                 bool allow_best_effort = true);

  private:
   DeviceConfiguration create_config(PhysicalDevice* device,
                                     const PhysicalDeviceRequirements& reqs);
   Instance* _instance{VK_NULL_HANDLE};
   Surface* _surface{VK_NULL_HANDLE};
};

}  // namespace meddl::render::vk
