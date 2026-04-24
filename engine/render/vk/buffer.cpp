#include "engine/render/vk/buffer.h"

#include <cstring>

namespace meddl::render::vk {

const VkVertexInputBindingDescription create_vertex_binding_description(size_t stride,
                                                                        uint32_t binding,
                                                                        VkVertexInputRate inputRate)
{
   VkVertexInputBindingDescription bindingDescription{};
   bindingDescription.binding = binding;
   bindingDescription.stride = static_cast<uint32_t>(stride);
   bindingDescription.inputRate = inputRate;
   return bindingDescription;
}

const std::array<VkVertexInputAttributeDescription, 4> create_vertex_attribute_descriptions(
    size_t position_offset,
    size_t color_offset,
    size_t normal_offset,
    size_t texcoord_offset,
    uint32_t binding)
{
   std::array<VkVertexInputAttributeDescription, 4> attributeDescriptions{};

   // Position attribute
   attributeDescriptions[0].binding = binding;
   attributeDescriptions[0].location = 0;
   attributeDescriptions[0].format = VK_FORMAT_R32G32B32_SFLOAT;
   attributeDescriptions[0].offset = static_cast<uint32_t>(position_offset);

   // Color attribute
   attributeDescriptions[1].binding = binding;
   attributeDescriptions[1].location = 1;
   attributeDescriptions[1].format = VK_FORMAT_R32G32B32_SFLOAT;
   attributeDescriptions[1].offset = static_cast<uint32_t>(color_offset);

   // Normal attribute
   attributeDescriptions[2].binding = binding;
   attributeDescriptions[2].location = 2;
   attributeDescriptions[2].format = VK_FORMAT_R32G32B32_SFLOAT;
   attributeDescriptions[2].offset = static_cast<uint32_t>(normal_offset);

   // Texture coordinate attribute
   attributeDescriptions[3].binding = binding;
   attributeDescriptions[3].location = 3;
   attributeDescriptions[3].format = VK_FORMAT_R32G32_SFLOAT;
   attributeDescriptions[3].offset = static_cast<uint32_t>(texcoord_offset);

   return attributeDescriptions;
}

Buffer::Buffer(Device* device,
               VkDeviceSize size,
               VkBufferUsageFlags usage,
               VkMemoryPropertyFlags properties)
    : _device(device), _size(size)
{
   VkBufferCreateInfo bufferInfo{};
   bufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
   bufferInfo.size = size;
   bufferInfo.usage = usage;
   bufferInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

   if (vkCreateBuffer(_device->vk(), &bufferInfo, nullptr, &_buffer) != VK_SUCCESS) {
      throw std::runtime_error("Failed to create buffer");
   }

   VkMemoryRequirements mem_req;
   vkGetBufferMemoryRequirements(_device->vk(), _buffer, &mem_req);

   VkMemoryAllocateInfo alloc_info{};
   alloc_info.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
   alloc_info.allocationSize = mem_req.size;
   alloc_info.memoryTypeIndex = find_memory_type(mem_req.memoryTypeBits, properties);

   // TODO:
   // It should be noted that in a real world application, you're not supposed to actually call
   // vkAllocateMemory for every individual buffer. The maximum number of simultaneous memory
   // allocations is limited by the maxMemoryAllocationCount physical device limit, which may be as
   // low as 4096 even on high end hardware like an NVIDIA GTX 1080. The right way to allocate
   // memory for a large number of objects at the same time is to create a custom allocator that
   // splits up a single allocation among many different objects by using the offset parameters that
   // we've seen in many functions.

   // You can either implement such an allocator yourself, or use the VulkanMemoryAllocator library
   // provided by the GPUOpen initiative. However, for this tutorial it's okay to use a separate
   // allocation for every resource, because we won't come close to hitting any of these limits for
   // now.

   if (vkAllocateMemory(_device->vk(), &alloc_info, nullptr, &_memory) != VK_SUCCESS) {
      throw std::runtime_error("Failed to allocate buffer memory");
   }

   vkBindBufferMemory(_device->vk(), _buffer, _memory, 0);
}

Buffer::~Buffer()
{
   if (_mapped_data) {
      unmap();
   }

   if (_buffer != VK_NULL_HANDLE) {
      vkDestroyBuffer(_device->vk(), _buffer, nullptr);
      _buffer = VK_NULL_HANDLE;
   }

   if (_memory != VK_NULL_HANDLE) {
      vkFreeMemory(_device->vk(), _memory, nullptr);
      _memory = VK_NULL_HANDLE;
   }
}

Buffer::Buffer(Buffer&& other) noexcept
    : _device(other._device),
      _buffer(other._buffer),
      _memory(other._memory),
      _size(other._size),
      _mapped_data(other._mapped_data)
{
   other._buffer = VK_NULL_HANDLE;
   other._memory = VK_NULL_HANDLE;
   other._size = 0;
   other._mapped_data = nullptr;
}

Buffer& Buffer::operator=(Buffer&& other) noexcept
{
   if (this != &other) {
      if (_buffer) vkDestroyBuffer(_device->vk(), _buffer, nullptr);
      if (_memory) vkFreeMemory(_device->vk(), _memory, nullptr);

      _device = other._device;
      _buffer = other._buffer;
      _memory = other._memory;
      _size = other._size;
      _mapped_data = other._mapped_data;

      other._buffer = VK_NULL_HANDLE;
      other._memory = VK_NULL_HANDLE;
      other._size = 0;
      other._mapped_data = nullptr;
   }
   return *this;
}

void Buffer::copy_from(Buffer* src, VkDeviceSize size)
{
   // Create a temporary command buffer
   VkCommandPool commandPool{};

   VkCommandPoolCreateInfo poolInfo{};
   poolInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
   poolInfo.flags = VK_COMMAND_POOL_CREATE_TRANSIENT_BIT;
   poolInfo.queueFamilyIndex = 0;  // TODO: Use graphics queue family index

   vkCreateCommandPool(_device->vk(), &poolInfo, nullptr, &commandPool);

   VkCommandBufferAllocateInfo allocInfo{};
   allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
   allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
   allocInfo.commandPool = commandPool;
   allocInfo.commandBufferCount = 1;

   VkCommandBuffer commandBuffer{};
   vkAllocateCommandBuffers(_device->vk(), &allocInfo, &commandBuffer);

   VkCommandBufferBeginInfo beginInfo{};
   beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
   beginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;

   vkBeginCommandBuffer(commandBuffer, &beginInfo);

   VkBufferCopy copyRegion{};
   copyRegion.size = size;
   vkCmdCopyBuffer(commandBuffer, src->vk(), _buffer, 1, &copyRegion);

   vkEndCommandBuffer(commandBuffer);

   VkSubmitInfo submitInfo{};
   submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
   submitInfo.commandBufferCount = 1;
   submitInfo.pCommandBuffers = &commandBuffer;

   // Submit to the graphics queue and wait until finished
   vkQueueSubmit(_device->queues().at(0).vk(), 1, &submitInfo, VK_NULL_HANDLE);
   vkQueueWaitIdle(_device->queues().at(0).vk());

   vkFreeCommandBuffers(_device->vk(), commandPool, 1, &commandBuffer);
   vkDestroyCommandPool(_device->vk(), commandPool, nullptr);
}

void Buffer::map()
{
   if (!_mapped_data) {
      vkMapMemory(_device->vk(), _memory, 0, _size, 0, &_mapped_data);
   }
}

void Buffer::unmap()
{
   if (_mapped_data) {
      vkUnmapMemory(_device->vk(), _memory);
      _mapped_data = nullptr;
   }
}

void Buffer::update(const void* data, VkDeviceSize size, VkDeviceSize offset)
{
   bool was_mapped = _mapped_data != nullptr;
   if (!was_mapped) {
      map();
   }

   std::memcpy(static_cast<char*>(_mapped_data) + offset, data, size);

   if (!was_mapped) {
      unmap();
   }
}

uint32_t Buffer::find_memory_type(uint32_t typeFilter, VkMemoryPropertyFlags properties)
{
   VkPhysicalDeviceMemoryProperties memory_properties;
   vkGetPhysicalDeviceMemoryProperties(_device->physical_device()->vk(), &memory_properties);

   for (uint32_t i = 0; i < memory_properties.memoryTypeCount; i++) {
      if ((typeFilter & (1 << i)) &&
          (memory_properties.memoryTypes[i].propertyFlags & properties) == properties) {
         return i;
      }
   }

   throw std::runtime_error("Failed to find suitable memory type");
}
}  // namespace meddl::render::vk
