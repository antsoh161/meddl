#pragma once

#include <vulkan/vulkan_core.h>

#include <cstdint>
#include <optional>

#include "GLFW/glfw3.h"
#include "wrappers/vulkan/surface_khr.h"

class PhysicalDevice {
  public:
   PhysicalDevice(VkPhysicalDevice handle, meddl::vulkan::SurfaceKHR& surface);
   PhysicalDevice(VkPhysicalDevice handle, VkSurfaceKHR& surface);
   operator VkPhysicalDevice() const;

   [[nodiscard]] std::optional<uint32_t> get_graphics_index() const;
   [[nodiscard]] std::optional<uint32_t> get_present_index() const;
   [[nodiscard]] std::optional<uint32_t> get_compute_index() const;
   [[nodiscard]] std::optional<uint32_t> get_transfer_index() const;

  private:
   struct QueueFamilyIndicies {
      std::optional<uint32_t> graphics;
      std::optional<uint32_t> present;
      std::optional<uint32_t> compute;
      std::optional<uint32_t> transfer;
   };
   VkPhysicalDevice m_handle;
   QueueFamilyIndicies m_family_indicies;
};
