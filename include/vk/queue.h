#pragma once

#include "GLFW/glfw3.h"
#include "vk/defaults.h"

namespace meddl::vk {

struct QueueConfiguration {
   QueueConfiguration(uint32_t queue_family_index,
                      float priority = defaults::DEFAULT_QUEUE_PRIORITY,
                      uint32_t queue_count = defaults::DEFAULT_QUEUE_COUNT)
       : _queue_family_index(queue_family_index), _priority(priority), _queue_count(queue_count)
   {
   }
   uint32_t _queue_family_index{};
   float _priority;
   uint32_t _queue_count;
};

class Queue {
  public:
   Queue() = delete;
   Queue(VkQueue queue, uint32_t queue_index, const QueueConfiguration& configuration);
   Queue(const Queue&) = delete;
   Queue& operator=(const Queue&) = delete;
   Queue(Queue&&) = default;
   Queue& operator=(Queue&&) = default;
   ~Queue() = default;

   [[nodiscard]] VkQueue vk() const { return _queue; }

  private:
   QueueConfiguration _configuration;
   uint32_t _queue_index{};
   VkQueue _queue;
};
}  // namespace meddl::vk
