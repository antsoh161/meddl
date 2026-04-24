#include "engine/loader.h"

#include <expected>
#include <filesystem>

#include "core/error.h"
#include "core/log.h"
#include "engine/types.h"

#define TINYGLTF_IMPLEMENTATION
#define STB_IMAGE_IMPLEMENTATION
#define STB_IMAGE_WRITE_IMPLEMENTATION
#include "tiny_gltf.h"

namespace meddl::loader {
namespace detail {

class Loader;  // fwd

template <typename T>
const T* accessor_data(const tinygltf::Model& model, int32_t accessor_index)
{
   const auto& accessor = model.accessors[accessor_index];
   const auto& buffer_view = model.bufferViews[accessor.bufferView];
   const auto& buffer = model.buffers[buffer_view.buffer];

   const uint8_t* data_ptr = buffer.data.data() + buffer_view.byteOffset + accessor.byteOffset;
   return std::bit_cast<const T*>(data_ptr);
}

void load_texture_from_image(const tinygltf::Model& model,
                             size_t image_index,
                             std::unordered_map<std::string, ImageData>& textures);
}  // namespace detail

std::expected<ImageData, error::Error> load_image(const std::filesystem::path& path)
{
   if (!std::filesystem::exists(path)) {
      return std::unexpected(
          error::Error(std::format("Can not load image, file not found: {}", path.string())));
   }

   int width{0}, height{0}, channels{0};
   stbi_set_flip_vertically_on_load(true);

   auto data = stbi_load(path.string().c_str(), &width, &height, &channels, STBI_rgb_alpha);

   if (!data) {
      return std::unexpected(
          error::Error(std::format("Can not load image, {}", stbi_failure_reason())));
   }

   ImageData image;
   image.width = width;
   image.height = height;
   image.channels = 4;
   image.pixels.resize(width * height * 4);
   std::memcpy(image.pixels.data(), data, width * height * 4);
   stbi_image_free(data);
   return image;
}

namespace detail {
class Loader {
  public:
   Loader(const std::filesystem::path& path, ModelLoadFlags flags, float scale_factor)
       : _path(path), _flags(flags), _scale_factor(scale_factor)
   {
   }

   std::expected<ModelData, error::Error> load()
   {
      tinygltf::TinyGLTF loader;
      std::string err;
      std::string warn;
      bool ret = false;

      if (_path.extension() == ".glb") {
         ret = loader.LoadBinaryFromFile(&_model, &err, &warn, _path.string());
      }
      else {
         ret = loader.LoadASCIIFromFile(&_model, &err, &warn, _path.string());
      }

      if (!warn.empty()) {
         log::warn("GLTF Warning: {}", warn);
      }

      if (!ret) {
         return std::unexpected(error::Error("Failed to load GLTF: " + err));
      }

      ModelData model_data;
      model_data.name = _path.stem().string();

      // Load model components based on flags
      if (has_flag(_flags, ModelLoadFlags::Meshes)) {
         if (!load_meshes(model_data)) {
            return std::unexpected(error::Error("Failed to load meshes"));
         }
      }

      if (has_flag(_flags, ModelLoadFlags::Materials)) {
         if (!load_materials(model_data)) {
            return std::unexpected(error::Error("Failed to load materials"));
         }
      }

      if (has_flag(_flags, ModelLoadFlags::Textures)) {
         if (!load_textures(model_data)) {
            return std::unexpected(error::Error("Failed to load textures"));
         }
      }

      if (has_flag(_flags, ModelLoadFlags::Nodes)) {
         if (!load_nodes(model_data)) {
            return std::unexpected(error::Error("Failed to load nodes"));
         }
      }

      if (has_flag(_flags, ModelLoadFlags::Animations)) {
         if (!load_animations(model_data)) {
            return std::unexpected(error::Error("Failed to load animations"));
         }
      }

      if (has_flag(_flags, ModelLoadFlags::Skins)) {
         if (!load_skins(model_data)) {
            return std::unexpected(error::Error("Failed to load skins"));
         }
      }

      return model_data;
   }

  private:
   bool load_meshes(ModelData& model_data)
   {
      if (_model.meshes.empty()) {
         return true;
      }
      for (const auto& mesh : _model.meshes) {
         MeshData mesh_data;
         mesh_data.name = mesh.name;

         for (const auto& primitive : mesh.primitives) {
            SubMesh submesh;
            submesh.index_offset = mesh_data.indices.size();
            submesh.material_index = primitive.material;

            if (primitive.attributes.find("POSITION") == primitive.attributes.end()) {
               log::warn("Skipping primitive without POSITION attribute");
               continue;
            }

            const auto* positions =
                accessor_data<float>(_model, primitive.attributes.at("POSITION"));
            const auto* normals =
                primitive.attributes.count("NORMAL")
                    ? accessor_data<float>(_model, primitive.attributes.at("NORMAL"))
                    : nullptr;
            const auto* texcoords =
                primitive.attributes.count("TEXCOORD_0")
                    ? accessor_data<float>(_model, primitive.attributes.at("TEXCOORD_0"))
                    : nullptr;
            const auto* tangents =
                primitive.attributes.count("TANGENT")
                    ? accessor_data<float>(_model, primitive.attributes.at("TANGENT"))
                    : nullptr;
            const auto* colors =
                primitive.attributes.count("COLOR_0")
                    ? accessor_data<float>(_model, primitive.attributes.at("COLOR_0"))
                    : nullptr;

            size_t vertex_count = _model.accessors[primitive.attributes.at("POSITION")].count;

            if (primitive.indices >= 0) {
               const auto& accessor = _model.accessors[primitive.indices];
               const auto& buffer_view = _model.bufferViews[accessor.bufferView];
               const auto& buffer = _model.buffers[buffer_view.buffer];

               size_t index_count = accessor.count;
               submesh.index_count = index_count;
               size_t old_size = mesh_data.indices.size();
               mesh_data.indices.resize(old_size + index_count);

               if (accessor.componentType == TINYGLTF_COMPONENT_TYPE_UNSIGNED_SHORT) {
                  const uint16_t* indices = reinterpret_cast<const uint16_t*>(
                      &buffer.data[buffer_view.byteOffset + accessor.byteOffset]);
                  std::copy(indices, indices + index_count, mesh_data.indices.begin() + old_size);
               }
               else if (accessor.componentType == TINYGLTF_COMPONENT_TYPE_UNSIGNED_INT) {
                  const uint32_t* indices = reinterpret_cast<const uint32_t*>(
                      &buffer.data[buffer_view.byteOffset + accessor.byteOffset]);
                  std::copy(indices, indices + index_count, mesh_data.indices.begin() + old_size);
               }
               else if (accessor.componentType == TINYGLTF_COMPONENT_TYPE_UNSIGNED_BYTE) {
                  const uint8_t* indices = reinterpret_cast<const uint8_t*>(
                      &buffer.data[buffer_view.byteOffset + accessor.byteOffset]);
                  for (size_t i = 0; i < index_count; i++) {
                     mesh_data.indices[old_size + i] = static_cast<uint32_t>(indices[i]);
                  }
               }
            }

            size_t old_vertex_count = mesh_data.vertices.size();
            mesh_data.vertices.resize(old_vertex_count + vertex_count);

            for (size_t i = 0; i < vertex_count; i++) {
               Vertex& v = mesh_data.vertices[old_vertex_count + i];

               if (positions) {
                  v.position = {positions[i * 3] * _scale_factor,
                                positions[i * 3 + 1] * _scale_factor,
                                positions[i * 3 + 2] * _scale_factor};
               }

               if (normals) {
                  v.normal = {normals[i * 3], normals[i * 3 + 1], normals[i * 3 + 2]};
               }

               if (texcoords) {
                  v.uv = {texcoords[i * 2], texcoords[i * 2 + 1]};
               }

               if (tangents) {
                  v.tangent = {tangents[i * 4],
                               tangents[i * 4 + 1],
                               tangents[i * 4 + 2],
                               tangents[i * 4 + 3]};
               }

               if (colors) {
                  // some models use 3 colors, some use 4
                  const auto& colorAccessor = _model.accessors[primitive.attributes.at("COLOR_0")];
                  if (colorAccessor.type == TINYGLTF_TYPE_VEC3) {
                     v.color = {colors[i * 3], colors[i * 3 + 1], colors[i * 3 + 2], 1.0f};
                  }
                  else if (colorAccessor.type == TINYGLTF_TYPE_VEC4) {
                     v.color = {
                         colors[i * 4], colors[i * 4 + 1], colors[i * 4 + 2], colors[i * 4 + 3]};
                  }
               }
               else {
                  v.color = {1.0f, 1.0f, 1.0f, 1.0f};  // Default white
               }
            }

            submesh.vertex_count = vertex_count;
            mesh_data.submeshes.push_back(submesh);
         }

         model_data.meshes.push_back(std::move(mesh_data));
         meddl::log::debug("Added one mesh...");
      }
      return true;
   }

   bool load_materials(ModelData& model_data)
   {
      for (const auto& mat : _model.materials) {
         MaterialData material;
         material.name = mat.name;

         if (mat.pbrMetallicRoughness.baseColorFactor.size() == 4) {
            material.diffuse_color = {
                static_cast<float>(mat.pbrMetallicRoughness.baseColorFactor[0]),
                static_cast<float>(mat.pbrMetallicRoughness.baseColorFactor[1]),
                static_cast<float>(mat.pbrMetallicRoughness.baseColorFactor[2]),
                static_cast<float>(mat.pbrMetallicRoughness.baseColorFactor[3])};
         }

         material.metallic = mat.pbrMetallicRoughness.metallicFactor;
         material.roughness = mat.pbrMetallicRoughness.roughnessFactor;

         if (mat.emissiveFactor.size() == 3) {
            material.emissive_factor = {static_cast<float>(mat.emissiveFactor[0]),
                                        static_cast<float>(mat.emissiveFactor[1]),
                                        static_cast<float>(mat.emissiveFactor[2])};
         }

         material.alpha_mode = mat.alphaMode == "MASK"    ? MaterialData::AlphaMode::Mask
                               : mat.alphaMode == "BLEND" ? MaterialData::AlphaMode::Blend
                                                          : MaterialData::AlphaMode::Opaque;
         material.alpha_cutoff = mat.alphaCutoff;
         material.double_sided = mat.doubleSided;

         // Texture references - we'll load the actual textures in the texture step
         if (mat.pbrMetallicRoughness.baseColorTexture.index >= 0) {
            const auto& texture = _model.textures[mat.pbrMetallicRoughness.baseColorTexture.index];
            material.albedo_texture = _model.images[texture.source].uri;
         }

         if (mat.pbrMetallicRoughness.metallicRoughnessTexture.index >= 0) {
            const auto& texture =
                _model.textures[mat.pbrMetallicRoughness.metallicRoughnessTexture.index];
            material.metallic_roughness_texture = _model.images[texture.source].uri;
         }

         if (mat.normalTexture.index >= 0) {
            const auto& texture = _model.textures[mat.normalTexture.index];
            material.normal_texture = _model.images[texture.source].uri;
            material.normal_scale = mat.normalTexture.scale;
         }

         if (mat.occlusionTexture.index >= 0) {
            const auto& texture = _model.textures[mat.occlusionTexture.index];
            material.occlusion_texture = _model.images[texture.source].uri;
            material.occlusion_strength = mat.occlusionTexture.strength;
         }

         if (mat.emissiveTexture.index >= 0) {
            const auto& texture = _model.textures[mat.emissiveTexture.index];
            material.emissive_texture = _model.images[texture.source].uri;
         }

         model_data.materials.push_back(std::move(material));
         meddl::log::debug("Added one material...");
      }
      return true;
   }

   bool load_textures(ModelData& model_data)
   {
      for (const auto& material : model_data.materials) {
         for (const auto& texture_ref : {material.albedo_texture,
                                         material.normal_texture,
                                         material.metallic_roughness_texture,
                                         material.occlusion_texture,
                                         material.emissive_texture}) {
            if (texture_ref.has_value() &&
                model_data.textures.find(texture_ref.value()) == model_data.textures.end()) {
               for (size_t i = 0; i < _model.images.size(); ++i) {
                  if (_model.images[i].uri == texture_ref.value()) {
                     load_texture_from_image(_model, i, model_data.textures);
                     meddl::log::debug("Loaded one texture: {}", _model.images[i].uri);
                     break;
                  }
               }
            }
         }
      }
      return true;
   }

   bool load_nodes(ModelData& model_data)
   {
      model_data.nodes.resize(_model.nodes.size());

      for (size_t i = 0; i < _model.nodes.size(); ++i) {
         const auto& src_node = _model.nodes[i];
         auto& dst_node = model_data.nodes[i];

         dst_node.name = src_node.name;
         dst_node.mesh_index = src_node.mesh;
         dst_node.skin_index = src_node.skin;

         if (!src_node.matrix.empty()) {
            // convert column-major matrix to our format
            constexpr size_t NODE_LENGTH = 16;
            for (int j = 0; j < NODE_LENGTH; ++j) {
               int col = j / 4;
               int row = j % 4;
               dst_node.transform[col][row] = static_cast<float>(src_node.matrix[j]);
            }
            if (_scale_factor != 1.0f) {
               dst_node.transform[3] *= _scale_factor;
               dst_node.transform[7] *= _scale_factor;
               dst_node.transform[11] *= _scale_factor;
            }
         }
         else {
            if (!src_node.translation.empty()) {
               dst_node.translation = {static_cast<float>(src_node.translation[0]) * _scale_factor,
                                       static_cast<float>(src_node.translation[1]) * _scale_factor,
                                       static_cast<float>(src_node.translation[2]) * _scale_factor};
            }

            if (!src_node.rotation.empty()) {
               dst_node.rotation = {static_cast<float>(src_node.rotation[0]),
                                    static_cast<float>(src_node.rotation[1]),
                                    static_cast<float>(src_node.rotation[2]),
                                    static_cast<float>(src_node.rotation[3])};
            }

            if (!src_node.scale.empty()) {
               dst_node.scale = {static_cast<float>(src_node.scale[0]),
                                 static_cast<float>(src_node.scale[1]),
                                 static_cast<float>(src_node.scale[2])};
            }
         }
      }

      for (size_t i = 0; i < _model.nodes.size(); ++i) {
         const auto& src_node = _model.nodes[i];
         auto& dst_node = model_data.nodes[i];

         for (const auto& child_idx : src_node.children) {
            dst_node.children.push_back(child_idx);
            model_data.nodes[child_idx].parent = i;
         }
      }

      if (!_model.scenes.empty()) {
         const auto& scene = _model.scenes[_model.defaultScene >= 0 ? _model.defaultScene : 0];
         model_data.root_nodes = scene.nodes;
      }

      return true;
   }

   bool load_animations(ModelData& model_data)
   {
      for (const auto& gltf_anim : _model.animations) {
         Animation anim;
         anim.name = gltf_anim.name;

         for (const auto& channel : gltf_anim.channels) {
            const auto& sampler = gltf_anim.samplers[channel.sampler];
            AnimationChannel anim_channel;

            anim_channel.node_index = channel.target_node;

            if (channel.target_path == "translation") {
               anim_channel.property = AnimationProperty::Translation;
            }
            else if (channel.target_path == "rotation") {
               anim_channel.property = AnimationProperty::Rotation;
            }
            else if (channel.target_path == "scale") {
               anim_channel.property = AnimationProperty::Scale;
            }
            else if (channel.target_path == "weights") {
               anim_channel.property = AnimationProperty::Weights;
            }
            else {
               continue;
            }

            const auto& input_accessor = _model.accessors[sampler.input];
            const auto* input_data = accessor_data<float>(_model, sampler.input);

            const auto& output_accessor = _model.accessors[sampler.output];
            const auto* output_data = accessor_data<float>(_model, sampler.output);

            size_t keyframe_count = input_accessor.count;

            if (sampler.interpolation == "LINEAR") {
               anim_channel.interpolation = AnimationInterpolation::Linear;
            }
            else if (sampler.interpolation == "STEP") {
               anim_channel.interpolation = AnimationInterpolation::Step;
            }
            else if (sampler.interpolation == "CUBICSPLINE") {
               anim_channel.interpolation = AnimationInterpolation::CubicSpline;
            }

            int components_per_keyframe = 0;
            if (anim_channel.property == AnimationProperty::Translation ||
                anim_channel.property == AnimationProperty::Scale) {
               components_per_keyframe = 3;
            }
            else if (anim_channel.property == AnimationProperty::Rotation) {
               components_per_keyframe = 4;
            }
            else if (anim_channel.property == AnimationProperty::Weights) {
               if (channel.target_node >= 0 && _model.nodes[channel.target_node].mesh >= 0) {
                  const auto& mesh = _model.meshes[_model.nodes[channel.target_node].mesh];
                  components_per_keyframe = mesh.weights.size();
               }
            }

            for (size_t i = 0; i < keyframe_count; ++i) {
               Keyframe kf;
               kf.time = input_data[i];

               for (int j = 0; j < components_per_keyframe; ++j) {
                  float value = output_data[i * components_per_keyframe + j];
                  if (anim_channel.property == AnimationProperty::Translation &&
                      _scale_factor != 1.0f) {
                     value *= _scale_factor;
                  }
                  kf.values.push_back(value);
               }

               anim_channel.keyframes.push_back(std::move(kf));
            }

            anim.channels.push_back(std::move(anim_channel));
         }

         model_data.animations.push_back(std::move(anim));
      }

      return true;
   }

   bool load_skins(ModelData& model_data)
   {
      for (const auto& gltf_skin : _model.skins) {
         Skin skin;
         skin.name = gltf_skin.name;

         skin.joints = gltf_skin.joints;

         if (gltf_skin.inverseBindMatrices >= 0) {
            const auto* ibm_data = accessor_data<float>(_model, gltf_skin.inverseBindMatrices);
            size_t matrix_count = _model.accessors[gltf_skin.inverseBindMatrices].count;

            skin.inverse_bind_matrices.resize(matrix_count * 16);
            for (size_t i = 0; i < matrix_count * 16; ++i) {
               float value = ibm_data[i];
               if (_scale_factor != 1.0f && (i % 16 == 3 || i % 16 == 7 || i % 16 == 11)) {
                  value *= _scale_factor;
               }
               skin.inverse_bind_matrices[i] = value;
            }
         }
         skin.skeleton_root = gltf_skin.skeleton;

         model_data.skins.push_back(std::move(skin));
      }

      return true;
   }
   std::filesystem::path _path;
   ModelLoadFlags _flags;
   float _scale_factor;
   tinygltf::Model _model;
};

template <typename T>
const T* get_accessor_data(const tinygltf::Model& model, int accessor_index)
{
   const auto& accessor = model.accessors[accessor_index];
   const auto& buffer_view = model.bufferViews[accessor.bufferView];
   const auto& buffer = model.buffers[buffer_view.buffer];

   return std::bit_cast<const T*>(&buffer.data[buffer_view.byteOffset + accessor.byteOffset]);
}

void load_texture_from_image(const tinygltf::Model& model,
                             size_t image_index,
                             std::unordered_map<std::string, ImageData>& textures)
{
   const auto& image = model.images[image_index];
   std::string key = image.uri;

   if (key.empty()) {
      key = "image_" + std::to_string(image_index);
   }

   if (textures.find(key) != textures.end()) {
      return;
   }

   ImageData img_data;
   img_data.width = image.width;
   img_data.height = image.height;
   img_data.channels = image.component;

   if (image.component == 3) {
      img_data.format_hint = "RGB8";
   }
   else if (image.component == 4) {
      img_data.format_hint = "RGBA8";
   }

   if (!image.image.empty()) {
      img_data.pixels.resize(image.image.size());
      std::memcpy(img_data.pixels.data(), image.image.data(), image.image.size());
   }
   else if (!image.uri.empty()) {
      if (image.uri.substr(0, 5) == "data:") {
         log::warn("Image data URI found but no image data available");
      }
      else {
         // @note: for now, store and load later
         img_data.uri = image.uri;
      }
   }

   textures[key] = std::move(img_data);
}
}  // namespace detail

std::expected<ModelData, error::Error> load_model(const std::filesystem::path& path,
                                                  ModelLoadFlags flags,
                                                  float scale_factor)
{
   if (!std::filesystem::exists(path)) {
      return std::unexpected(error::Error("File not found: " + path.string()));
   }

   if (path.extension() == ".gltf" || path.extension() == ".glb") {
      detail::Loader loader(path, flags, scale_factor);
      return loader.load();
   }
   else {
      return std::unexpected(
          error::Error("Unsupported model format: " + path.extension().string()));
   }
}

}  // namespace meddl::loader
