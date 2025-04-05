#include "engine/render/vk/command.h"

#include <vulkan/vulkan_core.h>

#include <expected>

namespace meddl::render::vk {

std::expected<CommandPool, error::Error> CommandPool::create(Device* device,
                                                             uint32_t queue_family_index,
                                                             VkCommandPoolCreateFlags flags)
{
   CommandPool pool;
   pool._device = device;
   VkCommandPoolCreateInfo create_info{};
   create_info.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
   create_info.queueFamilyIndex = queue_family_index;
   create_info.flags = flags;

   auto result = vkCreateCommandPool(
       pool._device->vk(), &create_info, pool._device->get_allocators(), &pool._command_pool);

   if (result != VK_SUCCESS) {
      return std::unexpected(error::Error(
          std::format("vkCreateCommandPool failed: {}", static_cast<int32_t>(result))));
   }
   return pool;
}

CommandPool::CommandPool(CommandPool&& other) noexcept
    : _command_pool(other._command_pool), _device(other._device)
{
   other._command_pool = VK_NULL_HANDLE;
   other._device = nullptr;
}

CommandPool& CommandPool::operator=(CommandPool&& other) noexcept
{
   if (this != &other) {
      _device = other._device;
      _command_pool = other._command_pool;
      other._command_pool = VK_NULL_HANDLE;
      other._device = nullptr;
   }
   return *this;
}

CommandPool::~CommandPool()
{
   if (_command_pool) {
      vkDestroyCommandPool(*_device, _command_pool, _device->get_allocators());
   }
}

std::expected<CommandBuffer, error::Error> CommandBuffer::create(
    Device* device, CommandPool* pool, const CommandBufferOptions& options)
{
   CommandBuffer buffer;
   buffer._device = device;
   buffer._pool = pool;
   VkCommandBufferAllocateInfo alloc_info{};
   alloc_info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
   alloc_info.commandPool = pool->vk();
   alloc_info.level = options.level;
   alloc_info.commandBufferCount = options.buffer_count;  // TODO: Configurable?

   auto result = vkAllocateCommandBuffers(*device, &alloc_info, &buffer._command_buffer);
   if (result != VK_SUCCESS) {
      return std::unexpected(error::Error(
          std::format("vkAllocateCommandBuffers failed: {}", static_cast<int32_t>(result))));
   }
   return buffer;
}

CommandBuffer::CommandBuffer(CommandBuffer&& other) noexcept
    : _device(other._device),
      _pool(other._pool),
      _command_buffer(other._command_buffer),
      _state(other._state)
{
   other._command_buffer = VK_NULL_HANDLE;
   other._device = nullptr;
   other._pool = nullptr;
   other._state = State::Ready;
}

CommandBuffer& CommandBuffer::operator=(CommandBuffer&& other) noexcept
{
   if (this != &other) {
      _device = other._device;
      _pool = other._pool;
      _command_buffer = other._command_buffer;
      _state = other._state;

      other._command_buffer = VK_NULL_HANDLE;
      other._device = nullptr;
      other._pool = nullptr;
      other._state = State::Ready;
   }
   return *this;
}

CommandBuffer::~CommandBuffer()
{
   if (_command_buffer) {
      vkFreeCommandBuffers(_device->vk(), _pool->vk(), 1, &_command_buffer);
   }
}

std::expected<void, error::Error> CommandBuffer::begin(VkCommandBufferUsageFlags flags)
{
   if (_state != State::Ready) {
      return std::unexpected(error::Error("Commandbuffer state is not ready"));
   }
   VkCommandBufferBeginInfo begin_info{};
   begin_info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
   begin_info.flags = flags;
   begin_info.pInheritanceInfo = nullptr;
   begin_info.pNext = nullptr;

   auto result = vkBeginCommandBuffer(_command_buffer, &begin_info);
   if (result != VK_SUCCESS) {
      return std::unexpected(error::Error(
          std::format("vkBeginCommandBuffer failed: {}", static_cast<int32_t>(result))));
   }
   _state = State::Recording;
   return {};
}

std::expected<void, error::Error> CommandBuffer::end()
{
   if (_state != State::Recording) {
      return std::unexpected(error::Error("Commandbuffer state is not recording"));
   }
   auto res = vkEndCommandBuffer(_command_buffer);
   if (res != VK_SUCCESS) {
      return std::unexpected(
          error::Error(std::format("vkEndCommandBuffer failed: {}", static_cast<int32_t>(res))));
   }
   _state = State::Executable;
   return {};
}

std::expected<void, error::Error> CommandBuffer::reset(VkCommandBufferResetFlags flags)
{
   if (_state != State::Executable) {
      return std::unexpected(error::Error("Commandbuffer state is not executable"));
   }
   auto res = vkResetCommandBuffer(_command_buffer, flags);
   if (res != VK_SUCCESS) {
      return std::unexpected(
          error::Error(std::format("vkResetCommandBuffer failed: {}", static_cast<int32_t>(res))));
   }
   _state = State::Ready;
   return {};
}

std::expected<void, error::Error> CommandBuffer::begin_renderpass(const RenderPass* renderpass,
                                                                  const Swapchain* swapchain,
                                                                  VkFramebuffer framebuffer)
{
   if (_state != State::Recording) {
      return std::unexpected(error::Error("Commandbuffer state is not recording"));
   }
   std::array<VkClearValue, 2> clear_values = {
       VkClearValue{.color = {{0.2f, 0.2f, 0.2f, 1.0f}}},  // Color clear value (RGBA)
       VkClearValue{.depthStencil = {.depth = 1.0f, .stencil = 0}}};

   VkRenderPassBeginInfo begin_info{};
   begin_info.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
   begin_info.renderPass = renderpass->vk();
   begin_info.framebuffer = framebuffer;
   begin_info.renderArea.offset = {.x = 0, .y = 0};
   begin_info.renderArea.extent = swapchain->extent();
   begin_info.clearValueCount = clear_values.size();
   begin_info.pClearValues = clear_values.data();

   vkCmdBeginRenderPass(_command_buffer, &begin_info, VK_SUBPASS_CONTENTS_INLINE);
   return {};
}

std::expected<void, error::Error> CommandBuffer::bind_pipeline(const GraphicsPipeline* pipeline)
{
   if (_state != State::Recording) {
      return std::unexpected(error::Error("Commandbuffer state is not recording"));
   }
   vkCmdBindPipeline(_command_buffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline->vk());
   return {};
}

std::expected<void, error::Error> CommandBuffer::set_viewport(const VkViewport& viewport)
{
   if (_state != State::Recording) {
      return std::unexpected(error::Error("Commandbuffer state is not recording"));
   }
   vkCmdSetViewport(_command_buffer, 0, 1, &viewport);
   return {};
}

std::expected<void, error::Error> CommandBuffer::set_scissor(const VkRect2D& scissor)
{
   if (_state != State::Recording) {
      return std::unexpected(error::Error("Commandbuffer state is not recording"));
   }
   vkCmdSetScissor(_command_buffer, 0, 1, &scissor);
   return {};
}

std::expected<void, error::Error> CommandBuffer::draw()
{
   if (_state != State::Recording) {
      return std::unexpected(error::Error("Commandbuffer state is not recording"));
   }
   vkCmdDraw(_command_buffer, 3, 1, 0, 0);
   return {};
}

std::expected<void, error::Error> CommandBuffer::end_renderpass()
{
   if (_state != State::Recording) {
      return std::unexpected(error::Error("Commandbuffer state is not recording"));
   }
   // TODO: error?
   vkCmdEndRenderPass(_command_buffer);
   return {};
}

std::expected<CommandBuffer, error::Error> CommandBuffer::begin_one_time_submit(Device* device,
                                                                                CommandPool* pool)
{
   CommandBufferOptions options;
   options.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
   options.buffer_count = 1;

   auto cmd_result = CommandBuffer::create(device, pool, options);
   if (!cmd_result) {
      return std::unexpected(cmd_result.error());
   }

   auto begin_result = cmd_result->begin(VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT);
   if (!begin_result) {
      return std::unexpected(begin_result.error());
   }
   return std::move(cmd_result.value());
}

std::expected<void, error::Error> CommandBuffer::end_and_submit(Device* device, CommandPool* pool)
{
   if (vkEndCommandBuffer(_command_buffer) != VK_SUCCESS) {
      return std::unexpected(error::Error("Failed to end command buffer recording"));
   }
   VkSubmitInfo info{};
   info.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
   info.commandBufferCount = 1;
   info.pCommandBuffers = &_command_buffer;

   auto& queue = device->queues().front();
   if (vkQueueSubmit(queue.vk(), 1, &info, VK_NULL_HANDLE) != VK_SUCCESS) {
      return std::unexpected(error::Error("Failed to submit command buffer"));
   }

   if (vkQueueWaitIdle(queue.vk()) != VK_SUCCESS) {
      return std::unexpected(error::Error("Failed to wait for queue idle"));
   }
   return {};
}

}  // namespace meddl::render::vk
