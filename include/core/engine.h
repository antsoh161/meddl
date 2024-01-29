#pragma once

#include <vector>

#include "core/logger.h"
#include "vulkan_renderer/device.h"
#include "vulkan_renderer/vulkan_debug.h"
#include "wrappers/glfw/glfw_wrapper.h"
#include "wrappers/vulkan/vulkan_wrapper.h"

struct QueueFamilyIndices {
   QueueFamilyIndices(VkPhysicalDevice device) {
      uint32_t n_queue_families = 0;
      vkGetPhysicalDeviceQueueFamilyProperties(device, &n_queue_families, nullptr);

      std::vector<VkQueueFamilyProperties> queue_families{n_queue_families};
      vkGetPhysicalDeviceQueueFamilyProperties(device, &n_queue_families, queue_families.data());

      for (int idx = 0; const auto &queue_family : queue_families) {
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
   std::vector<meddl::glfw::Window> m_windows;
   VkInstance m_instance{};
   // VkPhysicalDevice m_active_physical_device{VK_NULL_HANDLE};

   meddl::vulkan::SurfaceKHR m_surface;
   VkSurfaceKHR m_bajs_surface;

   std::vector<PhysicalDevice> m_physical_devices;
   std::unique_ptr<PhysicalDevice> m_active_physical_device;

   VkDevice m_active_logical_device{};
   VkQueue m_graphics_queue{};
   VkQueue m_present_queue{};

   VulkanDebugger m_debugger{};
   logger::AsyncLogger m_logger{};

   void create_physical_devices();
   void make_instance();
   const std::vector<const char *> get_required_extensions();
   void create_logical_device();
};
