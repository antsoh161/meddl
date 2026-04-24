#pragma once

#include <cstdint>
#include <expected>
#include <filesystem>
#include <span>
#include <string>

#include "core/error.h"
#include "types.h"

namespace meddl::loader {

enum class ModelLoadFlags : uint32_t {
   None = 0,
   Meshes = 1 << 0,
   Materials = 1 << 1,
   Textures = 1 << 2,
   Nodes = 1 << 3,
   Animations = 1 << 4,
   Skins = 1 << 5,

   Basic = Meshes,
   Standard = Meshes | Materials | Textures,
   Full = Meshes | Materials | Textures | Nodes | Animations | Skins,

   Default = Full
};

inline ModelLoadFlags operator|(ModelLoadFlags a, ModelLoadFlags b)
{
   return static_cast<ModelLoadFlags>(static_cast<uint32_t>(a) | static_cast<uint32_t>(b));
}

inline ModelLoadFlags operator&(ModelLoadFlags a, ModelLoadFlags b)
{
   return static_cast<ModelLoadFlags>(static_cast<uint32_t>(a) & static_cast<uint32_t>(b));
}

inline bool has_flag(ModelLoadFlags flags, ModelLoadFlags flag)
{
   return (flags & flag) == flag;
}

// Image loading functions
std::expected<ImageData, error::Error> load_image(const std::filesystem::path& path);
std::expected<ImageData, error::Error> load_image_from_memory(std::span<const uint8_t> data);

std::expected<ModelData, error::Error> load_model(const std::filesystem::path& path,
                                                  ModelLoadFlags flags = ModelLoadFlags::Default,
                                                  float scale_factor = 1.0f);

std::expected<ModelData, error::Error> load_gltf(const std::filesystem::path& path);
std::expected<ModelData, error::Error> load_obj(const std::filesystem::path& path);

// Shader loading functions
std::expected<ShaderData, error::Error> load_shader(const std::filesystem::path& path);
std::expected<ShaderData, error::Error> compile_glsl_to_spirv(const std::string& source,
                                                              const std::string& shader_type);

}  // namespace meddl::loader
