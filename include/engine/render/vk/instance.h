#pragma once

#include <memory>
#include <optional>

#include "GLFW/glfw3.h"
#include "engine/render/vk/debug.h"
#include "engine/render/vk/defaults.h"
#include "engine/render/vk/device.h"

namespace meddl::render::vk {
class PhysicalDevice;
class Instance {
  public:
   Instance(VkApplicationInfo app_info,
            const std::optional<DebugConfiguration>& debug_config = std::nullopt);
   ~Instance();
   Instance(const Instance&) = delete;
   Instance& operator=(const Instance&) = delete;

   Instance(Instance&&) = default;
   Instance& operator=(Instance&&) = default;

   [[nodiscard]] operator VkInstance() const { return _instance; }
   [[nodiscard]] VkInstance vk() const { return _instance; }

   [[nodiscard]] const std::vector<std::shared_ptr<PhysicalDevice>>& get_physical_devices() const;
   [[nodiscard]] Debugger* debugger() { return _debugger.get(); }

  private:
   VkInstance _instance{VK_NULL_HANDLE};
   std::unique_ptr<Debugger> _debugger;
   std::vector<std::shared_ptr<PhysicalDevice>> _physical_devices{};
};
}  // namespace meddl::render::vk
