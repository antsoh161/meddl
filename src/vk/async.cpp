#include "vk/async.h"

#include <vulkan/vulkan_core.h>

namespace meddl::vk {

template <typename T>
Lock<T>::Lock(Device* device, T& sync_obj, uint64_t timeout)
{
   if constexpr (std::is_same_v<T, Fence>) {
      sync_obj.wait(device, timeout);
      sync_obj.reset(device);
   }
}

Fence::Fence(Device* device) : _device{device}
{
   VkFenceCreateInfo fence_info{};
   fence_info.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
   fence_info.flags = 0;

   auto res = vkCreateFence(_device->vk(), &fence_info, nullptr, &_fence);
   if (res != VK_SUCCESS) {
      throw std::runtime_error{
          std::format("vkCreateFence failed with error: {}", static_cast<int32_t>(res))};
   }
}

void Fence::wait(Device* device, uint64_t timeout)
{
   vkWaitForFences(device->vk(), 1, &_fence, VK_TRUE, timeout);
}

void Fence::reset(Device* device)
{
   vkResetFences(device->vk(), 1, &_fence);
}

Fence::~Fence()
{
   if (_fence) {
      vkDestroyFence(_device->vk(), _fence, nullptr);
   }
}

Semaphore::Semaphore(Device* device) : _device{device}
{
   VkSemaphoreCreateInfo semaphore_info{};
   semaphore_info.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;
   auto res = vkCreateSemaphore(device->vk(), &semaphore_info, nullptr, &_semaphore);
   if (res != VK_SUCCESS) {
      throw std::runtime_error{
          std::format("vkCreateSemaphore failed with error: {}", static_cast<int32_t>(res))};
   }
}
Semaphore::~Semaphore()
{
   if (_semaphore) {
      vkDestroySemaphore(_device->vk(), _semaphore, nullptr);
   }
}
}  // namespace meddl::vk
