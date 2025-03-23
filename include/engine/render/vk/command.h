#pragma once
#include <expected>

#include "core/error.h"
#include "engine/render/vk/device.h"
#include "engine/render/vk/pipeline.h"
#include "engine/render/vk/renderpass.h"
#include "engine/render/vk/swapchain.h"

namespace meddl::render::vk {

struct CommandError : public meddl::error::Error {
   enum class Code {
      None,
      // vk error
      OutOfMemory,
      HostOutOfMemory,
      DeviceLost,
      // user error
      BufferNotReady,
      BufferNotRecording,
      BufferNotExecutable,
   } code;
   VkResult vk_result{VK_SUCCESS};
   CommandError(std::string_view msg,
                Code err_code,
                VkResult result = VK_SUCCESS,
                std::source_location loc = std::source_location::current())
       : Error(msg, loc), code(err_code), vk_result(result)
   {
   }
   static CommandError from_result(VkResult result, std::string_view operation)
   {
      Code code{};
      switch (result) {
         case VK_ERROR_OUT_OF_HOST_MEMORY:
            code = Code::HostOutOfMemory;
            break;
         case VK_ERROR_OUT_OF_DEVICE_MEMORY:
            code = Code::OutOfMemory;
            break;
         case VK_ERROR_DEVICE_LOST:
            code = Code::DeviceLost;
            break;
         default:
            code = Code::None;
            break;
      }

      std::string message =
          std::format("{} failed with {}", operation, static_cast<int32_t>(result));
      return {message, code, result};
   }
   static CommandError from_code(Code code, std::string_view operation)
   {
      std::string message = std::format("{} failed", operation);
      return {message, code};
   }
};

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
   uint32_t buffer_count{1};
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

   static CommandBuffer begin_one_time_submit(Device* device, CommandPool* pool);
   static void end_one_time_submit(Device* Device, CommandBuffer* cmd_buffer);

   //! The commands
   std::expected<void, CommandError> begin(
       VkCommandBufferUsageFlags flags = VK_COMMAND_BUFFER_LEVEL_PRIMARY);
   std::expected<void, CommandError> end();
   std::expected<void, CommandError> reset(VkCommandBufferResetFlags flags = 0);

   [[nodiscard]] State state() const { return _state; }

   //! Renderpass
   std::expected<void, CommandError> begin_renderpass(const RenderPass* renderpass,
                                                      const Swapchain* swapchain,
                                                      VkFramebuffer framebuffer);

   std::expected<void, CommandError> bind_pipeline(const GraphicsPipeline* pipeline);

   std::expected<void, CommandError> set_viewport(const VkViewport& viewport);

   std::expected<void, CommandError> set_scissor(const VkRect2D& scissor);

   std::expected<void, CommandError> draw();
   std::expected<void, CommandError> end_renderpass();

  private:
   Device* _device{nullptr};
   CommandPool* _pool{nullptr};
   VkCommandBuffer _command_buffer{VK_NULL_HANDLE};
   State _state{State::Ready};
};

}  // namespace meddl::render::vk
