#pragma once
#include "glm/ext/vector_float2.hpp"
#include "glm/glm.hpp"
namespace meddl::engine {

// Vulkan specific for now
struct Vertex {
   glm::vec3 position;
   glm::vec3 color;
   glm::vec3 normal;
   glm::vec2 texCoord;

   bool operator==(const Vertex& other) const = default;
};

// Vulkan specific for now
namespace vertex_layout {
constexpr size_t position_offset = offsetof(Vertex, position);
constexpr size_t color_offset = offsetof(Vertex, color);
constexpr size_t normal_offset = offsetof(Vertex, normal);
constexpr size_t texcoord_offset = offsetof(Vertex, texCoord);
constexpr size_t stride = sizeof(Vertex);

}  // namespace vertex_layout
}  // namespace meddl::engine
