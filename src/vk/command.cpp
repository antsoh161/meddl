#include "vk/command.h"

#include <exception>

#include "vk/error.h"

namespace meddl::vk {

//! Command Pool
CommandPool::CommandPool(Device* device,
                         uint32_t queue_family_index,
                         VkCommandPoolCreateFlags flags)
    : _device(device)
{
   VkCommandPoolCreateInfo create_info{};
   create_info.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
   create_info.queueFamilyIndex = queue_family_index;
   create_info.flags = flags;

   if (vkCreateCommandPool(*device, &create_info, device->get_allocators(), &_command_pool) !=
       VK_SUCCESS) {
      throw std::runtime_error("failed to create command pool");
   }
}

CommandPool::~CommandPool()
{
   if (_command_pool) {
      vkDestroyCommandPool(*_device, _command_pool, _device->get_allocators());
   }
}

//! CommandBuffer
CommandBuffer::CommandBuffer(Device* device, CommandPool* pool, const CommandBufferOptions& options)
    : _device(device), _pool(pool)
{
   VkCommandBufferAllocateInfo alloc_info{};
   alloc_info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
   alloc_info.commandPool = pool->vk();
   alloc_info.level = options.level;
   alloc_info.commandBufferCount = options.buffer_count;  // TODO: Configurable?

   if (vkAllocateCommandBuffers(*device, &alloc_info, &_command_buffer) != VK_SUCCESS) {
      throw std::runtime_error("failed to allocate command buffer");
   }
}

CommandBuffer::~CommandBuffer()
{
   if (_command_buffer) {
      vkFreeCommandBuffers(_device->vk(), _pool->vk(), 1, &_command_buffer);
   }
}

std::expected<void, CommandError> CommandBuffer::begin(VkCommandBufferUsageFlags flags)
{
   if (_state != State::Ready) {
      return std::unexpected(meddl::vk::CommandError::NotReady);
   }
   VkCommandBufferBeginInfo begin_info{};
   begin_info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
   begin_info.flags = flags;
   begin_info.pInheritanceInfo = nullptr;
   begin_info.pNext = nullptr;

   if (vkBeginCommandBuffer(_command_buffer, &begin_info) != VK_SUCCESS) {
      throw std::runtime_error("failed to begin recording command buffer");
   }
   _state = State::Recording;
   return {};
}

std::expected<void, CommandError> CommandBuffer::end()
{
   if (_state != State::Recording) {
      return std::unexpected(meddl::vk::CommandError::NotRecording);
   }
   if (vkEndCommandBuffer(_command_buffer) != VK_SUCCESS) {
      throw std::runtime_error("failed to record command buffer");
   }
   _state = State::Executable;
   return {};
}

std::expected<void, CommandError> CommandBuffer::reset(VkCommandBufferResetFlags flags)
{
   if (_state != State::Executable) {
      return std::unexpected(meddl::vk::CommandError::NotExecutable);
   }
   if (vkResetCommandBuffer(_command_buffer, flags) != VK_SUCCESS) {
      throw std::runtime_error("failed to reset command buffer");
   }
   _state = State::Ready;
   return {};
}

std::expected<void, CommandError> CommandBuffer::begin_renderpass(const RenderPass* renderpass,
                                                                  const Swapchain* swapchain,
                                                                  VkFramebuffer framebuffer)
{
   if (_state != State::Recording) {
      return std::unexpected(meddl::vk::CommandError::NotRecording);
   }
   VkClearValue clear_values = {{{0.0f, 0.0f, 0.0f, 1.0f}}};

   VkRenderPassBeginInfo begin_info{};
   begin_info.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
   begin_info.renderPass = renderpass->vk();
   begin_info.framebuffer = framebuffer;
   begin_info.renderArea.offset = {0, 0};
   begin_info.renderArea.extent = swapchain->extent();
   begin_info.clearValueCount = 1;
   begin_info.pClearValues = &clear_values;

   // TODO: error?
   vkCmdBeginRenderPass(_command_buffer, &begin_info, VK_SUBPASS_CONTENTS_INLINE);
   return {};
}

std::expected<void, CommandError> CommandBuffer::bind_pipeline(const GraphicsPipeline* pipeline)
{
   if (_state != State::Recording) {
      return std::unexpected(meddl::vk::CommandError::NotRecording);
   }
   // TODO: error?
   vkCmdBindPipeline(_command_buffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline->vk());
   return {};
}

std::expected<void, CommandError> CommandBuffer::set_viewport(const VkViewport& viewport)
{
   if (_state != State::Recording) {
      return std::unexpected(CommandError::NotRecording);
   }
   // TODO: error?
   vkCmdSetViewport(_command_buffer, 0, 1, &viewport);
   return {};
}

std::expected<void, CommandError> CommandBuffer::set_scissor(const VkRect2D& scissor)
{
   if (_state != State::Recording) {
      return std::unexpected(CommandError::NotRecording);
   }
   // TODO: error?
   vkCmdSetScissor(_command_buffer, 0, 1, &scissor);
   return {};
}

std::expected<void, CommandError> CommandBuffer::draw()
{
   return {};
}

std::expected<void, CommandError> CommandBuffer::end_renderpass()
{
   if (_state != State::Recording) {
      return std::unexpected(meddl::vk::CommandError::NotRecording);
   }
   // TODO: error?
   vkCmdEndRenderPass(_command_buffer);
   return {};
}

}  // namespace meddl::vk
