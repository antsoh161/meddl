#pragma once

#include <vulkan/vulkan_core.h>

#include <unordered_set>

#include "GLFW/glfw3.h"

namespace meddl::render::vk {

enum class QueueFamilyType { Graphics, Present, Compute, Transfer, Unknown };

struct QueueConfiguration {
   QueueConfiguration(uint32_t queue_family_index, float priority = 1, uint32_t queue_count = 1)
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
   std::unordered_set<QueueFamilyType> _types{};
};

}  // namespace meddl::render::vk
