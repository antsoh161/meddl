#pragma once

#include <unordered_set>

#include "GLFW/glfw3.h"
#include "engine/render/vk/device.h"
#include "engine/render/vk/hash.hpp"
#include "engine/render/vk/image.h"
#include "engine/render/vk/renderpass.h"
#include "engine/render/vk/shared.h"
#include "engine/window.h"

namespace meddl::render::vk {

class Swapchain {
  public:
   Swapchain() = delete;
   // Swapchain(Device* device,
   //           Surface* surface,
   //           const RenderPass* renderpass,
   //           const SwapchainOptions& options,
   //           const glfw::FrameBufferSize& fbs);
   Swapchain(Device* device,
             Surface* surface,
             const RenderPass* renderpass,
             const GraphicsConfiguration& config,
             const glfw::FrameBufferSize& fbs);
   ~Swapchain();

   Swapchain(const Swapchain&) = delete;
   Swapchain& operator=(const Swapchain&) = delete;
   Swapchain(Swapchain&&) = default;
   Swapchain& operator=(Swapchain&&) = default;

   // [[nodiscard]] std::vector<VkImageView>& get_image_views();

   [[nodiscard]] VkExtent2D extent() const { return _extent2d; }
   [[nodiscard]] VkSwapchainKHR vk() const { return _swapchain; }
   [[nodiscard]] const GraphicsConfiguration& config() { return _config; }

   [[nodiscard]] const std::vector<VkFramebuffer>& get_framebuffers() const
   {
      return _framebuffers;
   }

   static std::unique_ptr<Swapchain> recreate(Device* device,
                                              Surface* surface,
                                              const RenderPass* renderpass,
                                              const glfw::FrameBufferSize& fbs,
                                              std::unique_ptr<Swapchain> old_swapchain);

  private:
   VkSwapchainKHR _swapchain{VK_NULL_HANDLE};
   Device* _device;
   Surface* _surface;
   GraphicsConfiguration _config;

   VkExtent2D _extent2d{};
   std::unordered_set<VkSurfaceFormatKHR> _formats{};
   std::unordered_set<VkPresentModeKHR> _present_modes{};
   std::optional<Image> _depth_image{std::nullopt};
   // std::vector<VkImage> _images{};
   std::vector<Image> _images{};
   // std::vector<VkImageView> _image_views{};
   std::vector<VkFramebuffer> _framebuffers{};
};
}  // namespace meddl::render::vk
