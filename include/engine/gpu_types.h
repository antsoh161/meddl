#pragma once
#include "glm/glm.hpp"
namespace meddl {

struct Vertex {
   glm::vec3 position;
   glm::vec4 color;
   glm::vec3 normal;
   glm::vec2 uv;
   glm::vec4 tangent;
   bool operator==(const Vertex& other) const = default;
};

namespace vertex_layout {
constexpr size_t position_offset = offsetof(Vertex, position);
constexpr size_t color_offset = offsetof(Vertex, color);
constexpr size_t normal_offset = offsetof(Vertex, normal);
constexpr size_t uv_offset = offsetof(Vertex, uv);
constexpr size_t tangent_offset = offsetof(Vertex, tangent);
constexpr size_t stride = sizeof(Vertex);

}  // namespace vertex_layout

struct TransformUBO {
   glm::mat4 model;
   glm::mat4 view;
   glm::mat4 projection;
};

namespace transform_layout {
static_assert(sizeof(glm::mat4) == sizeof(float) * 4 * 4, "bad glm::mat4 size");
constexpr size_t model_offset = offsetof(TransformUBO, model);
static_assert(model_offset == 0, "bad model offset");

constexpr size_t view_offset = offsetof(TransformUBO, view);
static_assert(view_offset == sizeof(float) * 4 * 4, "bad view offset ");

constexpr size_t projection_offset = offsetof(TransformUBO, projection);
static_assert(projection_offset == sizeof(float) * 4 * 4 * 2, "bad projection offset");

constexpr size_t stride = sizeof(TransformUBO);
static_assert(stride == sizeof(float) * 4 * 4 * 3);  // mat4 * 3
}  // namespace transform_layout

struct MaterialUBO {
   glm::vec4 diffuse_color;
   float shininess;
   float metallic;
   float roughness;
   float ao;
};

namespace material_layout {
constexpr size_t diffuse_offset = offsetof(MaterialUBO, diffuse_color);
constexpr size_t shininess_offset = offsetof(MaterialUBO, shininess);
constexpr size_t metallic_offset = offsetof(MaterialUBO, metallic);
constexpr size_t roughness_offset = offsetof(MaterialUBO, roughness);
constexpr size_t ao_offset = offsetof(MaterialUBO, ao);
}  // namespace material_layout

struct Mesh {
   uint32_t index_count;
   uint32_t index_offset;
   uint32_t vertex_count;
   uint32_t vertex_offset;
   uint32_t material_index;
};

namespace mesh_layout {
constexpr size_t index_count_offset = offsetof(Mesh, index_count);
constexpr size_t index_offset_offset = offsetof(Mesh, index_offset);
constexpr size_t vertex_count_offset = offsetof(Mesh, vertex_count);
constexpr size_t vertex_offset_offset = offsetof(Mesh, vertex_offset);
constexpr size_t material_index_offset = offsetof(Mesh, material_index);
constexpr size_t stride = sizeof(Mesh);
}  // namespace mesh_layout

}  // namespace meddl
