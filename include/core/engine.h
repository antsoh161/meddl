#pragma once

#include <vulkan/vulkan_core.h>
#define GLFW_INCLUDE_VULKAN
#include <vector>

#include "core/logger.h"
#include "core/window.h"
#include "vulkan_renderer/vulkan_debug.h"

struct QueueFamilyIndices {
  QueueFamilyIndices(VkPhysicalDevice device) {
    uint32_t n_queue_families = 0;
    vkGetPhysicalDeviceQueueFamilyProperties(device, &n_queue_families, nullptr);

    std::vector<VkQueueFamilyProperties> queue_families{n_queue_families};
    vkGetPhysicalDeviceQueueFamilyProperties(device, &n_queue_families, queue_families.data());

    for (int idx = 0; const auto& queue_family : queue_families) {
      if (queue_family.queueFlags & VK_QUEUE_GRAPHICS_BIT) {
        m_graphics = idx;
      }
      idx++;
    }
  }
  std::optional<uint32_t> m_graphics;
  std::optional<uint32_t> m_present;
  std::optional<uint32_t> m_compute;
  std::optional<uint32_t> m_transfer;

  [[nodiscard]] bool has_all() const {
    return m_graphics.has_value() && m_present.has_value() && m_compute.has_value() &&
           m_transfer.has_value();
  }
};

class Engine {
 public:
  Engine() = default;
  ~Engine();
  Engine(Engine &&) = delete;
  Engine(const Engine &) = delete;
  Engine &operator=(Engine &&) = delete;
  Engine &operator=(const Engine &) = delete;

  void init_glfw();
  void init_vulkan();
  void run();

 private:
  std::vector<Window> m_windows;
  VkInstance m_instance{};
  VkPhysicalDevice m_active_physical_device{VK_NULL_HANDLE};

  VkDevice m_active_logical_device{};

  VulkanDebugger m_debugger{};
  logger::AsyncLogger m_logger{};

  void make_instance();
  const std::vector<const char *> get_required_extensions();
  std::vector<VkPhysicalDevice> get_suitable_devices();
  void create_logical_device();
};
