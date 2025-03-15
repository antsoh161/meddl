#pragma once

#include <expected>

#include "core/error.h"
#include "vk/defaults.h"
#include "vk/device.h"
#include "vk/pipeline.h"
#include "vk/renderpass.h"
#include "vk/swapchain.h"

namespace meddl::vk {

//! CommandPool
class CommandPool {
  public:
   CommandPool() = delete;
   CommandPool(Device* device, uint32_t queue_family_index, VkCommandPoolCreateFlags flags = 0);
   virtual ~CommandPool();

   // Non-copyable
   CommandPool(const CommandPool&) = delete;
   CommandPool& operator=(const CommandPool&) = delete;

   // Movable
   CommandPool(CommandPool&& other) noexcept;
   CommandPool& operator=(CommandPool&& other) noexcept;

   [[nodiscard]] VkCommandPool vk() const { return _command_pool; }

  private:
   VkCommandPool _command_pool{VK_NULL_HANDLE};
   Device* _device;
};

struct CommandBufferOptions {
   VkCommandBufferLevel level{VK_COMMAND_BUFFER_LEVEL_PRIMARY};
   uint32_t buffer_count{defaults::DEFAULT_COMMAND_BUFFER_COUNT};
};

//! CommandBuffer
class CommandBuffer {
  public:
   enum class State : uint8_t { Ready, Recording, Executable };

   CommandBuffer() = delete;
   CommandBuffer(Device* device, CommandPool* pool, const CommandBufferOptions& options = {});
   virtual ~CommandBuffer();

   // Non-copyable
   CommandBuffer(const CommandBuffer&) = delete;
   CommandBuffer& operator=(const CommandBuffer&) = delete;

   // Movable
   CommandBuffer(CommandBuffer&& other) noexcept;
   CommandBuffer& operator=(CommandBuffer&& other) noexcept;

   [[nodiscard]] VkCommandBuffer vk() const { return _command_buffer; }

   //! The commands
   std::expected<void, meddl::error::VulkanError> begin(
       VkCommandBufferUsageFlags flags = defaults::DEFAULT_BUFFER_USAGE_FLAGS);
   std::expected<void, meddl::error::VulkanError> end();
   std::expected<void, meddl::error::VulkanError> reset(VkCommandBufferResetFlags flags = 0);

   [[nodiscard]] const State state() const { return _state; }

   //! Renderpass
   std::expected<void, meddl::error::VulkanError> begin_renderpass(const RenderPass* renderpass,
                                                                   const Swapchain* swapchain,
                                                                   VkFramebuffer framebuffer);

   std::expected<void, meddl::error::VulkanError> bind_pipeline(const GraphicsPipeline* pipeline);

   std::expected<void, meddl::error::VulkanError> set_viewport(const VkViewport& viewport);

   std::expected<void, meddl::error::VulkanError> set_scissor(const VkRect2D& scissor);

   std::expected<void, meddl::error::VulkanError> draw();
   std::expected<void, meddl::error::VulkanError> end_renderpass();

  private:
   Device* _device{nullptr};
   CommandPool* _pool{nullptr};
   VkCommandBuffer _command_buffer{VK_NULL_HANDLE};
   State _state{State::Ready};
};

}  // namespace meddl::vk
