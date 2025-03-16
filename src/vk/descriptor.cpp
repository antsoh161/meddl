#include "vk/descriptor.h"

#include <stdexcept>

namespace meddl::vk {

DescriptorSetLayout::DescriptorSetLayout(Device* device, VkDescriptorSetLayoutBinding binding)
    : DescriptorSetLayout(device, std::vector<VkDescriptorSetLayoutBinding>{binding})
{
}

DescriptorSetLayout::DescriptorSetLayout(Device* device,
                                         const std::vector<VkDescriptorSetLayoutBinding>& bindings)
    : _device(device)
{
   VkDescriptorSetLayoutCreateInfo layout_info{};
   layout_info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
   layout_info.bindingCount = static_cast<uint32_t>(bindings.size());
   layout_info.pBindings = bindings.data();

   if (vkCreateDescriptorSetLayout(_device->vk(), &layout_info, nullptr, &_layout) != VK_SUCCESS) {
      throw std::runtime_error("Failed to create descriptor set layout");
   }
}

DescriptorSetLayout::~DescriptorSetLayout()
{
   if (_layout != VK_NULL_HANDLE) {
      vkDestroyDescriptorSetLayout(_device->vk(), _layout, nullptr);
   }
}

DescriptorSetLayout::DescriptorSetLayout(DescriptorSetLayout&& other) noexcept
    : _device(other._device), _layout(other._layout)
{
   other._layout = VK_NULL_HANDLE;
}

DescriptorSetLayout& DescriptorSetLayout::operator=(DescriptorSetLayout&& other) noexcept
{
   if (this != &other) {
      if (_layout != VK_NULL_HANDLE) {
         vkDestroyDescriptorSetLayout(_device->vk(), _layout, nullptr);
      }
      _device = other._device;
      _layout = other._layout;
      other._layout = VK_NULL_HANDLE;
   }
   return *this;
}

DescriptorPool::DescriptorPool(Device* device,
                               uint32_t max_sets,
                               const std::vector<VkDescriptorPoolSize>& pool_sizes)
    : _device(device), _pool_sizes(pool_sizes)
{
   VkDescriptorPoolCreateInfo pool_info{};
   pool_info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
   pool_info.poolSizeCount = static_cast<uint32_t>(_pool_sizes.size());
   pool_info.pPoolSizes = _pool_sizes.data();
   pool_info.maxSets = max_sets;

   if (vkCreateDescriptorPool(_device->vk(), &pool_info, nullptr, &_pool) != VK_SUCCESS) {
      throw std::runtime_error("Failed to create descriptor pool");
   }
}

DescriptorPool::~DescriptorPool()
{
   if (_pool) {
      vkDestroyDescriptorPool(_device->vk(), _pool, nullptr);
   }
}

DescriptorPool::DescriptorPool(DescriptorPool&& other) noexcept
    : _device(other._device), _pool(other._pool)
{
   other._pool = VK_NULL_HANDLE;
}

DescriptorPool& DescriptorPool::operator=(DescriptorPool&& other) noexcept
{
   if (this != &other) {
      if (_pool) {
         vkDestroyDescriptorPool(_device->vk(), _pool, nullptr);
      }
      _device = other._device;
      _pool = other._pool;
      other._pool = VK_NULL_HANDLE;
   }
   return *this;
}

DescriptorSet::DescriptorSet(Device* device, DescriptorPool* pool, DescriptorSetLayout* layout)
    : _device(device)
{
   VkDescriptorSetLayout layout_handle = layout->vk();

   VkDescriptorSetAllocateInfo alloc_info{};
   alloc_info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
   alloc_info.descriptorPool = pool->vk();
   alloc_info.descriptorSetCount = 1;
   alloc_info.pSetLayouts = &layout_handle;

   if (vkAllocateDescriptorSets(_device->vk(), &alloc_info, &_set) != VK_SUCCESS) {
      throw std::runtime_error("Failed to allocate descriptor set");
   }
}

void DescriptorSet::update(uint32_t binding,
                           VkBuffer buffer,
                           VkDeviceSize offset,
                           VkDeviceSize range)
{
   VkDescriptorBufferInfo buffer_info{};
   buffer_info.buffer = buffer;
   buffer_info.offset = offset;
   buffer_info.range = range;

   VkWriteDescriptorSet descriptor_write{};
   descriptor_write.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
   descriptor_write.dstSet = _set;
   descriptor_write.dstBinding = binding;
   descriptor_write.dstArrayElement = 0;
   descriptor_write.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
   descriptor_write.descriptorCount = 1;
   descriptor_write.pBufferInfo = &buffer_info;

   vkUpdateDescriptorSets(_device->vk(), 1, &descriptor_write, 0, nullptr);
}

}  // namespace meddl::vk
