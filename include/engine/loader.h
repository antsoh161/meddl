#pragma once

#include <cstdint>
#include <filesystem>
#include <optional>
#include <span>
#include <string>
#include <unordered_map>
#include <vector>

#include "gpu_types.h"

namespace meddl::engine {
using AssetID = uint32_t;

struct ImageData {
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
   uint32_t index_offset{0};
   uint32_t index_count{0};
   uint32_t material_index{0};
};

struct MeshData {
   std::vector<Vertex> vertices;
   std::vector<uint32_t> indicies;
   std::vector<SubMesh> submeshes;
   [[nodiscard]] std::vector<engine::Mesh> to_gpu_meshes() const
   {
      std::vector<engine::Mesh> result;
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
   std::string name;
   glm::vec4 diffuse_color{1.0f};
   float shininess{32.0f};
   float metallic{0.0f};
   float roughness{1.0f};
   float ao{1.0f};

   std::optional<std::string> base_color_texture;
   std::optional<std::string> normal_texture;
   std::optional<std::string> metallic_roughness_texture;
   std::optional<std::string> occlusion_texture;
   std::optional<std::string> emissive_texture;

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

struct ModelData {
   std::string name;
   std::vector<MeshData> meshes;
   std::vector<MaterialData> materials;
   std::unordered_map<std::string, ImageData> textures;
};

struct ShaderData {
   std::vector<uint32_t> spirv_code;
   std::string entry_point{"main"};
   std::string shader_type;  // "vertex", "fragment", etc.
};

namespace loader {
// Image loading functions
ImageData load_image(const std::filesystem::path& path);
ImageData load_image_from_memory(std::span<const uint8_t> data);

// Model loading functions
ModelData load_model(const std::filesystem::path& path);
ModelData load_gltf(const std::filesystem::path& path);
ModelData load_obj(const std::filesystem::path& path);

// Shader loading functions
ShaderData load_shader(const std::filesystem::path& path);
ShaderData compile_glsl_to_spirv(const std::string& source, const std::string& shader_type);
}  // namespace loader

}  // namespace meddl::engine
