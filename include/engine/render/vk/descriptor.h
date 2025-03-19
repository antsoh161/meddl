#pragma once

#include <vulkan/vulkan.h>

#include <vector>

#include "engine/render/vk/device.h"

namespace meddl::render::vk {

class DescriptorSetLayout {
  public:
   DescriptorSetLayout(Device* device, VkDescriptorSetLayoutBinding binding);
   DescriptorSetLayout(Device* device, const std::vector<VkDescriptorSetLayoutBinding>& bindings);
   ~DescriptorSetLayout();

   DescriptorSetLayout(const DescriptorSetLayout&) = delete;
   DescriptorSetLayout& operator=(const DescriptorSetLayout&) = delete;

   DescriptorSetLayout(DescriptorSetLayout&& other) noexcept;
   DescriptorSetLayout& operator=(DescriptorSetLayout&& other) noexcept;

   [[nodiscard]] VkDescriptorSetLayout vk() const { return _layout; }
   [[nodiscard]] const VkDescriptorSetLayout* vk_ptr() const { return &_layout; }

  private:
   Device* _device;
   VkDescriptorSetLayout _layout = VK_NULL_HANDLE;
};

class DescriptorPool {
  public:
   DescriptorPool(Device* device,
                  uint32_t max_sets,
                  const std::vector<VkDescriptorPoolSize>& pool_sizes);
   ~DescriptorPool();

   DescriptorPool(const DescriptorPool&) = delete;
   DescriptorPool& operator=(const DescriptorPool&) = delete;

   DescriptorPool(DescriptorPool&& other) noexcept;
   DescriptorPool& operator=(DescriptorPool&& other) noexcept;

   [[nodiscard]] VkDescriptorPool vk() const { return _pool; }
   [[nodiscard]] const VkDescriptorPool* vk_ptr() const { return &_pool; }

  private:
   Device* _device;
   VkDescriptorPool _pool = VK_NULL_HANDLE;
   std::vector<VkDescriptorPoolSize> _pool_sizes{};
};

class DescriptorSet {
  public:
   DescriptorSet(Device* device, DescriptorPool* pool, DescriptorSetLayout* layout);

   void update(uint32_t binding, VkBuffer buffer, VkDeviceSize offset, VkDeviceSize range);

   [[nodiscard]] VkDescriptorSet vk() const { return _set; }
   [[nodiscard]] const VkDescriptorSet* vk_ptr() const { return &_set; }

  private:
   Device* _device;
   VkDescriptorSet _set = VK_NULL_HANDLE;
};
}  // namespace meddl::vk
