#pragma once

#include <vulkan/vulkan_core.h>

#include <array>

#include "engine/render/vk/device.h"
namespace meddl::render::vk {

const VkVertexInputBindingDescription create_vertex_binding_description(
    size_t stride, uint32_t binding = 0, VkVertexInputRate inputRate = VK_VERTEX_INPUT_RATE_VERTEX);

const std::array<VkVertexInputAttributeDescription, 4> create_vertex_attribute_descriptions(
    size_t position_offset,
    size_t color_offset,
    size_t normal_offset,
    size_t texcoord_offset,
    uint32_t binding = 0);

class Device;
class Buffer {
  public:
   Buffer(Device* device,
          VkDeviceSize size,
          VkBufferUsageFlags usage,
          VkMemoryPropertyFlags properties);
   ~Buffer();

   Buffer(const Buffer&) = delete;
   Buffer& operator=(const Buffer&) = delete;

   Buffer(Buffer&& other) noexcept;
   Buffer& operator=(Buffer&& other) noexcept;

   void copy_from(Buffer* src, VkDeviceSize size);

   void map();
   void unmap();
   void update(const void* data, VkDeviceSize size, VkDeviceSize offset = 0);

   [[nodiscard]] VkBuffer vk() const { return _buffer; }
   [[nodiscard]] VkDeviceMemory memory() const { return _memory; }
   [[nodiscard]] VkDeviceSize size() const { return _size; }
   [[nodiscard]] void* mapped_data() const { return _mapped_data; }
   [[nodiscard]] bool is_mapped() const { return _mapped_data != nullptr; }

  private:
   Device* _device;
   VkBuffer _buffer = VK_NULL_HANDLE;
   VkDeviceMemory _memory = VK_NULL_HANDLE;
   VkDeviceSize _size = 0;
   void* _mapped_data = nullptr;

   uint32_t find_memory_type(uint32_t typeFilter, VkMemoryPropertyFlags properties);
};
}  // namespace meddl::vk
