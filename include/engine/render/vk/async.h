#pragma once
#include <limits>

#include "GLFW/glfw3.h"
#include "engine/render/vk/device.h"
namespace meddl::render::vk {

class Fence {
  public:
   Fence() = delete;
   Fence(Device* device);
   ~Fence();
   Fence(Fence&&) noexcept;
   Fence& operator=(Fence&&) noexcept;
   Fence(const Fence&) = delete;
   Fence& operator=(const Fence&) = delete;

   [[nodiscard]] VkFence vk() const { return _fence; }
   void wait(Device* device, uint64_t timeout = UINT64_MAX);
   void reset(Device* device);

  private:
   Device* _device{VK_NULL_HANDLE};
   VkFence _fence{VK_NULL_HANDLE};
};

class Semaphore {
  public:
   Semaphore() = delete;
   Semaphore(Device* device);
   ~Semaphore();
   Semaphore(const Semaphore&) = delete;
   Semaphore& operator=(const Semaphore&) = delete;
   Semaphore(Semaphore&&) noexcept;
   Semaphore& operator=(Semaphore&&) noexcept;

   [[nodiscard]] VkSemaphore vk() const { return _semaphore; }

  private:
   Device* _device{VK_NULL_HANDLE};
   VkSemaphore _semaphore{VK_NULL_HANDLE};
};

// TODO: This is not an appropriate implementation, revise.
// lock for fences..?
template <typename T>
class Lock {
  public:
   Lock() = delete;
   explicit Lock(Device* device,
                 T& sync_t,
                 uint64_t timeout = std::numeric_limits<uint64_t>::max());
   ~Lock();
   Lock(const Lock&) = delete;
   Lock& operator=(const Lock&) = delete;
   Lock(Lock&&) = default;
   Lock& operator=(Lock&&) = default;
};

// deduction guides
Lock(Device*, Fence&) -> Lock<Fence>;
Lock(Device*, Semaphore&) -> Lock<Semaphore>;

}  // namespace meddl::render::vk
