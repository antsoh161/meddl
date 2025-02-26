#pragma once

#include <unordered_set>

#include "GLFW/glfw3.h"
#include "vk/device.h"
#include "app/window.h"
#include "vk/hash.hpp"
#include "vk/renderpass.h"

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

//! New
class Swapchain {
  public:
   Swapchain() = delete;
   Swapchain(PhysicalDevice* physical_device,
                Device* device,
                Surface* surface,
                const RenderPass* renderpass,
                const SwapchainOptions& options,
                const glfw::FrameBufferSize& fbs);
   ~Swapchain();

   Swapchain(const Swapchain&) = delete;
   Swapchain& operator=(const Swapchain&) = delete;
   Swapchain(Swapchain&&) = default;
   Swapchain& operator=(Swapchain&&) = default;

   [[nodiscard]] std::vector<VkImageView>& get_image_views();

   [[nodiscard]] VkExtent2D extent() const { return _extent2d; }
   [[nodiscard]] VkSwapchainKHR vk() const { return _swapchain; }

   [[nodiscard]] const std::vector<VkFramebuffer>& get_framebuffers() const { return _framebuffers; }

  private:
   VkSwapchainKHR _swapchain{VK_NULL_HANDLE};
   Device* _device;
   Surface* _surface;

   SwapchainDetails _details{};
   VkExtent2D _extent2d{};
   std::unordered_set<VkSurfaceFormatKHR> _formats{};
   std::unordered_set<VkPresentModeKHR> _present_modes{};
   std::vector<VkImage> _images{};
   std::vector<VkImageView> _image_views{};
   std::vector<VkFramebuffer> _framebuffers{};
};
}  // namespace meddl::vk
