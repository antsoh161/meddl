#include "engine/render/vk/async.h"

#include <vulkan/vulkan_core.h>

namespace meddl::render::vk {

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
   fence_info.flags = VK_FENCE_CREATE_SIGNALED_BIT;

   auto res = vkCreateFence(_device->vk(), &fence_info, nullptr, &_fence);
   if (res != VK_SUCCESS) {
      throw std::runtime_error{
          std::format("vkCreateFence failed with error: {}", static_cast<int32_t>(res))};
   }
}

Fence::Fence(Fence&& other) noexcept : _device{other._device}, _fence{other._fence}
{
   other._device = VK_NULL_HANDLE;
   other._fence = VK_NULL_HANDLE;
}

Fence& Fence::operator=(Fence&& other) noexcept
{
   if (this != &other) {
      if (_fence != VK_NULL_HANDLE) {
         vkDestroyFence(_device->vk(), _fence, nullptr);
      }
      _device = other._device;
      _fence = other._fence;
      other._device = VK_NULL_HANDLE;
      other._fence = VK_NULL_HANDLE;
   }
   return *this;
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

Semaphore::Semaphore(Semaphore&& other) noexcept
    : _device{other._device}, _semaphore{other._semaphore}
{
   other._device = VK_NULL_HANDLE;
   other._semaphore = VK_NULL_HANDLE;
}

Semaphore& Semaphore::operator=(Semaphore&& other) noexcept
{
   if (this != &other) {
      if (_semaphore != VK_NULL_HANDLE) {
         vkDestroySemaphore(_device->vk(), _semaphore, nullptr);
      }
      _device = other._device;
      _semaphore = other._semaphore;
      other._device = VK_NULL_HANDLE;
      other._semaphore = VK_NULL_HANDLE;
   }
   return *this;
}

Semaphore::~Semaphore()
{
   if (_semaphore) {
      vkDestroySemaphore(_device->vk(), _semaphore, nullptr);
   }
}
}  // namespace meddl::render::vk
