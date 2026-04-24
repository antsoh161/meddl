#pragma once

#include <cstdint>
#include <optional>
#include <string>
#include <vector>

#include "glm/glm.hpp"
#include "glm/gtc/quaternion.hpp"
#include "gpu_types.h"
namespace meddl {

using AssetID = uint32_t;

struct ImageData {
   std::optional<std::string> uri;
   std::vector<uint8_t> pixels;
   uint32_t width{0};
   uint32_t height{0};
   uint32_t channels{0};
   std::string format_hint;  // e.g. "RGBA8", "BC7", etc.
   bool generate_mipmaps{true};

   // Helper methods
   [[nodiscard]] size_t size_bytes() const { return pixels.size(); }
};

// TODO: Do any of these need a gpu_type?
struct SubMesh {
   uint32_t vertex_count{0};
   uint32_t index_offset{0};
   uint32_t index_count{0};
   uint32_t material_index{0};
};

struct MeshData {
   std::string name{};
   std::vector<Vertex> vertices;
   std::vector<uint32_t> indices;
   std::vector<SubMesh> submeshes;
   [[nodiscard]] std::vector<Mesh> to_gpu_meshes() const
   {
      std::vector<Mesh> result;
      for (const auto& submesh : submeshes) {
         Mesh mesh{};
         mesh.index_count = submesh.index_count;
         mesh.index_offset = submesh.index_offset;
         mesh.vertex_count = 0;  // TODO: Not 0..
         mesh.vertex_offset = 0;
         mesh.material_index = submesh.material_index;
         result.push_back(mesh);
      }
      return result;
   }
};

struct MaterialData {
   enum class AlphaMode {
      Opaque,
      Mask,
      Blend,
   };
   std::string name;
   glm::vec4 diffuse_color{1.0f};
   glm::vec3 emissive_factor{1.0f};
   AlphaMode alpha_mode{AlphaMode::Mask};
   double alpha_cutoff{0.5};
   double normal_scale{1.0f};
   double occlusion_strength{1.0f};
   bool double_sided{false};

   float shininess{1.0f};
   float metallic{0.0f};
   float roughness{1.0f};
   float ao{1.0f};

   std::optional<std::string> base_color_texture;
   std::optional<std::string> normal_texture;
   std::optional<std::string> metallic_roughness_texture;
   std::optional<std::string> occlusion_texture;
   std::optional<std::string> emissive_texture;
   std::optional<std::string> albedo_texture;

   // Conversion to GPU material
   [[nodiscard]] MaterialUBO to_material_ubo() const
   {
      MaterialUBO ubo{};
      ubo.diffuse_color = diffuse_color;
      ubo.shininess = shininess;
      ubo.metallic = metallic;
      ubo.roughness = roughness;
      ubo.ao = ao;
      return ubo;
   }
};

struct Node {
   std::string name;
   int32_t parent{-1};
   std::vector<uint32_t> children;

   glm::mat4 transform{1.0f};

   glm::vec3 translation{0.0f};
   glm::quat rotation{1.0f, 0.0f, 0.0f, 0.0f};
   glm::vec3 scale{1.0f};

   int32_t mesh_index{-1};
   int32_t skin_index{-1};
};

struct Skin {
   std::string name;
   std::vector<int32_t> joints;
   std::vector<float> inverse_bind_matrices;
   int32_t skeleton_root{-1};
};

enum class AnimationProperty { Translation, Rotation, Scale, Weights };

enum class AnimationInterpolation { Linear, Step, CubicSpline };

struct Keyframe {
   float time;
   std::vector<float> values;
};

struct AnimationChannel {
   uint32_t node_index;
   AnimationProperty property;
   AnimationInterpolation interpolation{AnimationInterpolation::Linear};
   std::vector<Keyframe> keyframes;
};

struct Animation {
   std::string name;
   std::vector<AnimationChannel> channels;
};

struct ModelData {
   std::string name;
   std::vector<MeshData> meshes;
   std::vector<MaterialData> materials;
   std::unordered_map<std::string, ImageData> textures;
   std::vector<Animation> animations;
   std::vector<Node> nodes;
   std::vector<int32_t> root_nodes;
   std::vector<Skin> skins;
};

struct ShaderData {
   std::vector<uint32_t> spirv_code;
   std::string entry_point{"main"};
   std::string shader_type;  // "vertex", "fragment", etc.
};

}  // namespace meddl
