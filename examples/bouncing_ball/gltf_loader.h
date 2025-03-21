#include <filesystem>

#include "core/log.h"
#include "engine/gpu_types.h"
#include "tiny_gltf.h"

class ModelLoader {
  public:
   ModelLoader() = default;

   bool load(const std::string& filepath, float scale_factor = 1.0f)
   {
      // Clear previous data
      _vertices.clear();
      _indices.clear();

      tinygltf::Model model;
      tinygltf::TinyGLTF loader;
      std::string err, warn;

      bool ret = filepath.ends_with(".glb")
                     ? loader.LoadBinaryFromFile(&model, &err, &warn, filepath)
                     : loader.LoadASCIIFromFile(&model, &err, &warn, filepath);

      if (!warn.empty()) meddl::log::warn("GLTF warning: {}", warn);
      if (!err.empty()) meddl::log::error("GLTF error: {}", err);

      if (!ret) {
         meddl::log::error("Failed to load model from: {}", filepath);
         return false;
      }

      // Compute bounding box to normalize/center the model
      glm::vec3 min_bounds(std::numeric_limits<float>::max());
      glm::vec3 max_bounds(std::numeric_limits<float>::lowest());

      // Process all meshes in the model
      for (const auto& mesh : model.meshes) {
         for (const auto& primitive : mesh.primitives) {
            process_primitive(model, primitive, min_bounds, max_bounds);
         }
      }

      // Calculate model dimensions and center point
      glm::vec3 dimensions = max_bounds - min_bounds;
      glm::vec3 center = (min_bounds + max_bounds) * 0.5f;

      // Scale and center the model
      float max_dim = std::max({dimensions.x, dimensions.y, dimensions.z});
      float normalize_factor = 1.0f / max_dim;

      // Apply scaling and centering to all vertices
      for (auto& vertex : _vertices) {
         // Center the model at origin
         vertex.position -= center;

         // Scale the model to fit in a 1x1x1 cube, then apply user scale factor
         vertex.position *= normalize_factor * scale_factor;
      }

      meddl::log::info(
          "Loaded model with {} vertices and {} indices", _vertices.size(), _indices.size());
      return !_vertices.empty();
   }

   const std::vector<meddl::engine::Vertex>& vertices() const { return _vertices; }
   const std::vector<uint32_t>& indices() const { return _indices; }

  private:
   void process_primitive(const tinygltf::Model& model,
                          const tinygltf::Primitive& primitive,
                          glm::vec3& min_bounds,
                          glm::vec3& max_bounds)
   {
      // Get accessor indices
      auto pos_it = primitive.attributes.find("POSITION");
      if (pos_it == primitive.attributes.end()) {
         meddl::log::error("No position data in mesh");
         return;
      }

      uint32_t vertex_start = static_cast<uint32_t>(_vertices.size());

      // Get position data
      const tinygltf::Accessor& pos_accessor = model.accessors[pos_it->second];
      const tinygltf::BufferView& pos_view = model.bufferViews[pos_accessor.bufferView];
      const tinygltf::Buffer& pos_buffer = model.buffers[pos_view.buffer];

      const float* positions = reinterpret_cast<const float*>(
          &pos_buffer.data[pos_view.byteOffset + pos_accessor.byteOffset]);

      // Check for other attributes
      auto norm_it = primitive.attributes.find("NORMAL");
      auto tex_it = primitive.attributes.find("TEXCOORD_0");
      auto color_it = primitive.attributes.find("COLOR_0");

      // Access normals if available
      const float* normals = nullptr;
      if (norm_it != primitive.attributes.end()) {
         const tinygltf::Accessor& norm_accessor = model.accessors[norm_it->second];
         const tinygltf::BufferView& norm_view = model.bufferViews[norm_accessor.bufferView];
         const tinygltf::Buffer& norm_buffer = model.buffers[norm_view.buffer];
         normals = reinterpret_cast<const float*>(
             &norm_buffer.data[norm_view.byteOffset + norm_accessor.byteOffset]);
      }

      // Access texture coordinates if available
      const float* texcoords = nullptr;
      if (tex_it != primitive.attributes.end()) {
         const tinygltf::Accessor& tex_accessor = model.accessors[tex_it->second];
         const tinygltf::BufferView& tex_view = model.bufferViews[tex_accessor.bufferView];
         const tinygltf::Buffer& tex_buffer = model.buffers[tex_view.buffer];
         texcoords = reinterpret_cast<const float*>(
             &tex_buffer.data[tex_view.byteOffset + tex_accessor.byteOffset]);
      }

      // Access colors if available
      const float* colors = nullptr;
      if (color_it != primitive.attributes.end()) {
         const tinygltf::Accessor& color_accessor = model.accessors[color_it->second];
         const tinygltf::BufferView& color_view = model.bufferViews[color_accessor.bufferView];
         const tinygltf::Buffer& color_buffer = model.buffers[color_view.buffer];
         colors = reinterpret_cast<const float*>(
             &color_buffer.data[color_view.byteOffset + color_accessor.byteOffset]);
      }

      // Process all vertices
      size_t vertex_count = pos_accessor.count;
      for (size_t i = 0; i < vertex_count; i++) {
         meddl::engine::Vertex vertex{};

         // Position
         vertex.position.x = positions[i * 3];
         vertex.position.y = positions[i * 3 + 1];
         vertex.position.z = positions[i * 3 + 2];

         // Update bounds for normalization
         min_bounds.x = std::min(min_bounds.x, vertex.position.x);
         min_bounds.y = std::min(min_bounds.y, vertex.position.y);
         min_bounds.z = std::min(min_bounds.z, vertex.position.z);
         max_bounds.x = std::max(max_bounds.x, vertex.position.x);
         max_bounds.y = std::max(max_bounds.y, vertex.position.y);
         max_bounds.z = std::max(max_bounds.z, vertex.position.z);

         // Normal
         if (normals) {
            vertex.normal.x = normals[i * 3];
            vertex.normal.y = normals[i * 3 + 1];
            vertex.normal.z = normals[i * 3 + 2];
         }
         else {
            vertex.normal = {0.0f, 0.0f, 1.0f};
         }

         // Texture coordinates
         if (texcoords) {
            vertex.texCoord.x = texcoords[i * 2];
            vertex.texCoord.y = texcoords[i * 2 + 1];
         }
         else {
            vertex.texCoord = {0.0f, 0.0f};
         }

         // Color
         if (colors) {
            vertex.color.r = colors[i * 4];
            vertex.color.g = colors[i * 4 + 1];
            vertex.color.b = colors[i * 4 + 2];
         }
         else {
            vertex.color = {1.0f, 1.0f, 1.0f};
         }

         _vertices.push_back(vertex);
      }

      // Process indices if present
      if (primitive.indices >= 0) {
         const tinygltf::Accessor& index_accessor = model.accessors[primitive.indices];
         const tinygltf::BufferView& index_view = model.bufferViews[index_accessor.bufferView];
         const tinygltf::Buffer& index_buffer = model.buffers[index_view.buffer];

         const uint8_t* index_data =
             &index_buffer.data[index_view.byteOffset + index_accessor.byteOffset];

         size_t index_count = index_accessor.count;

         // Handle different index component types
         if (index_accessor.componentType == TINYGLTF_COMPONENT_TYPE_UNSIGNED_SHORT) {
            const uint16_t* indices = reinterpret_cast<const uint16_t*>(index_data);
            for (size_t i = 0; i < index_count; i++) {
               _indices.push_back(vertex_start + static_cast<uint32_t>(indices[i]));
            }
         }
         else if (index_accessor.componentType == TINYGLTF_COMPONENT_TYPE_UNSIGNED_INT) {
            const uint32_t* indices = reinterpret_cast<const uint32_t*>(index_data);
            for (size_t i = 0; i < index_count; i++) {
               _indices.push_back(vertex_start + indices[i]);
            }
         }
         else if (index_accessor.componentType == TINYGLTF_COMPONENT_TYPE_UNSIGNED_BYTE) {
            const uint8_t* indices = index_data;
            for (size_t i = 0; i < index_count; i++) {
               _indices.push_back(vertex_start + static_cast<uint32_t>(indices[i]));
            }
         }
      }
      else {
         // If no indices provided, create sequential indices for triangle list
         for (uint32_t i = 0; i < vertex_count; i++) {
            _indices.push_back(vertex_start + i);
         }
      }
   }

   std::vector<meddl::engine::Vertex> _vertices;
   std::vector<uint32_t> _indices;
};
