#pragma once

#include "GLFW/glfw3.h"
#include "vk/device.h"
#include "shader.h"

namespace meddl::vk {

class GraphicsPipeline {
  public:
   GraphicsPipeline(const ShaderModule& vert_shader, const ShaderModule& frag_shader, Device* device);
  private:
   VkPipeline _pipeline;
   Device* _device;
};
}  // namespace meddl::vk
