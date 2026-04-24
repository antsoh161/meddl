#pragma once
#include <expected>

#include "core/error.h"
#include "engine/render/vk/device.h"
#include "engine/render/vk/pipeline.h"
#include "engine/render/vk/renderpass.h"
#include "engine/render/vk/swapchain.h"

namespace meddl::render::vk {

//! CommandPool
class CommandPool {
  public:
   CommandPool() = default;
   static std::expected<CommandPool, error::Error> create(Device* device,
                                                          uint32_t queue_family_index,
                                                          VkCommandPoolCreateFlags flags = 0);
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
   Device* _device{nullptr};
};

struct CommandBufferOptions {
   VkCommandBufferLevel level{VK_COMMAND_BUFFER_LEVEL_PRIMARY};
   uint32_t buffer_count{1};
};

//! CommandBuffer
class CommandBuffer {
  public:
   enum class State : uint8_t { Ready, Recording, Executable };

   CommandBuffer() = default;

   static std::expected<CommandBuffer, error::Error> create(
       Device* device, CommandPool* pool, const CommandBufferOptions& options = {});

   virtual ~CommandBuffer();

   // Non-copyable
   CommandBuffer(const CommandBuffer&) = delete;
   CommandBuffer& operator=(const CommandBuffer&) = delete;

   // Movable
   CommandBuffer(CommandBuffer&& other) noexcept;
   CommandBuffer& operator=(CommandBuffer&& other) noexcept;

   [[nodiscard]] VkCommandBuffer vk() const { return _command_buffer; }

   //! The commands
   std::expected<void, error::Error> begin(
       VkCommandBufferUsageFlags flags = VK_COMMAND_BUFFER_LEVEL_PRIMARY);
   std::expected<void, error::Error> end();
   std::expected<void, error::Error> reset(VkCommandBufferResetFlags flags = 0);

   [[nodiscard]] State state() const { return _state; }

   //! Renderpass
   std::expected<void, error::Error> begin_renderpass(const RenderPass* renderpass,
                                                      const Swapchain* swapchain,
                                                      VkFramebuffer framebuffer);
   std::expected<void, error::Error> bind_pipeline(const GraphicsPipeline* pipeline);
   std::expected<void, error::Error> set_viewport(const VkViewport& viewport);
   std::expected<void, error::Error> set_scissor(const VkRect2D& scissor);
   std::expected<void, error::Error> draw();
   std::expected<void, error::Error> end_renderpass();

   //! One time submits
   //! @note end_and_submit must be called on the return CommandBuffer
   static std::expected<CommandBuffer, error::Error> begin_one_time_submit(Device* device,
                                                                           CommandPool* pool);
   std::expected<void, error::Error> end_and_submit(Device* device, CommandPool* pool);

  private:
   Device* _device{nullptr};
   CommandPool* _pool{nullptr};
   VkCommandBuffer _command_buffer{VK_NULL_HANDLE};
   State _state{State::Ready};
};

}  // namespace meddl::render::vk
