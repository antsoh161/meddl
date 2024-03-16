#include "vk/queue.h"

namespace meddl::vk {

Queue::Queue(VkQueue queue, uint32_t queue_index, const QueueConfiguration& configuration)
    : _configuration(configuration), _queue_index(queue_index), _queue(queue)
{
}

// Queue::~Queue(){
// }
}  // namespace meddl::vk
