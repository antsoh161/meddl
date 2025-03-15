#pragma once

#include <memory>
#include <optional>

#include "GLFW/glfw3.h"
#include "vk/debug.h"
#include "vk/defaults.h"
#include "vk/device.h"

namespace meddl::vk {
class PhysicalDevice;
class Instance {
  public:
   Instance(VkApplicationInfo app_info,
            VkDebugUtilsMessengerCreateInfoEXT debug_info,
            std::optional<Debugger>& debugger);
   ~Instance();
   Instance(const Instance&) = delete;
   Instance& operator=(const Instance&) = delete;
   // TODO: Movable?
   Instance(Instance&&) = delete;
   Instance& operator=(Instance&&) = delete;

   [[nodiscard]] operator VkInstance() const { return _instance; }
   [[nodiscard]] VkInstance vk() const { return _instance; }

   [[nodiscard]] const std::vector<std::shared_ptr<PhysicalDevice>>& get_physical_devices() const;

  private:
   VkInstance _instance{VK_NULL_HANDLE};
   std::vector<std::shared_ptr<PhysicalDevice>> _physical_devices{};
};
}  // namespace meddl::vk
