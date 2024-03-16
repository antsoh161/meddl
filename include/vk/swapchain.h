#pragma once

#include <optional>
#include <unordered_set>

#include "GLFW/glfw3.h"
#include "vk/device.h"
#include "wrappers/glfw/window.h"
#include "wrappers/vulkan/vulkan_hash.hpp"

namespace meddl::vk {

// Options struct with default value if left unspecified
struct SwapchainOptions {
   VkSurfaceFormatKHR _surface_format = {defaults::DEFAULT_IMAGE_FORMAT,
                                         defaults::DEFAULT_IMAGE_COLOR_SPACE};
   VkPresentModeKHR _present_mode = {defaults::DEFAULT_PRESENT_MODE};
   VkImageUsageFlags _image_usage_flags = defaults::DEFAULT_IMAGE_USAGE_FLAGS;
   uint32_t _image_count{defaults::DEFAULT_IMAGE_COUNT};
   uint32_t _image_array_layers{defaults::DEFAULT_IMAGE_ARRAY_LAYERS};
   bool _clipped{defaults::DEFAULT_SWAPCHAIN_CLIPPED};
};

struct SwapchainDetails {
   VkSurfaceCapabilitiesKHR _capabilities{};
   std::unordered_set<VkSurfaceFormatKHR> _formats{};
   std::unordered_set<VkPresentModeKHR> _present_modes{};
};

//! Swapchain representation
class SwapChain {
  public:
   SwapChain() = default;

   SwapChain(LogicalDevice& logical_device,
             const std::unordered_set<VkSurfaceFormatKHR>& formats,
             const std::unordered_set<VkPresentModeKHR>& present_modes,
             const VkSurfaceCapabilitiesKHR& surface_capabilities,
             const VkSwapchainCreateInfoKHR& swapchain_info);

   operator VkSwapchainKHR();
   [[nodiscard]] constexpr size_t get_image_count() const;
   [[nodiscard]] std::vector<VkImageView>& get_image_views();

  private:
   SwapchainDetails _details;
   VkSwapchainCreateInfoKHR _active_swapchain_info{};
   std::vector<VkImage> _images{};
   std::vector<VkImageView> _image_views{};

   VkSwapchainKHR _handle{VK_NULL_HANDLE};
};

//! New
class NewSwapchain {
  public:
   NewSwapchain() = delete;
   NewSwapchain(NewPhysicalDevice* physical_device,
                NewDevice* device,
                Surface* surface,
                const SwapchainOptions& options,
                const glfw::FrameBufferSize& fbs);
   ~NewSwapchain();

   NewSwapchain(const NewSwapchain&) = delete;
   NewSwapchain& operator=(const NewSwapchain&) = delete;
   NewSwapchain(NewSwapchain&&) = default;
   NewSwapchain& operator=(NewSwapchain&&) = default;

   [[nodiscard]] std::vector<VkImageView>& get_image_views();

  private:
   VkSwapchainKHR _swapchain{VK_NULL_HANDLE};
   NewDevice* _device;
   Surface* _surface;

   SwapchainDetails _details{};
   std::unordered_set<VkSurfaceFormatKHR> _formats{};
   std::unordered_set<VkPresentModeKHR> _present_modes{};
   std::vector<VkImage> _images{};
   std::vector<VkImageView> _image_views{};
};
}  // namespace meddl::vk
