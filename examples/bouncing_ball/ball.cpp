
#include <filesystem>
#include <memory>
#include <print>
#include <shaderc/shaderc.hpp>
#include <stdexcept>
#include <thread>

#include "GLFW/glfw3.h"
#include "core/formatter_utils.h"
#include "core/log.h"
#include "engine/events/event.h"
#include "engine/events/keycodes.h"
#include "engine/gpu_types.h"
#include "engine/renderer.h"
#include "engine/window.h"
#include "glm/ext/matrix_transform.hpp"
#include "spdlog/common.h"

#define TINYGLTF_IMPLEMENTATION
#define STB_IMAGE_IMPLEMENTATION
#define STB_IMAGE_WRITE_IMPLEMENTATION
#include "gltf_loader.h"

struct Ball {
   glm::vec3 position{0.0f, 0.0f, 0.0f};
   glm::vec3 velocity{0.01f, 0.008f, 0.0f};
   float radius{0.15f};
   glm::vec3 color{1.0f, 0.3f, 0.2f};

   [[nodiscard]] std::vector<meddl::engine::Vertex> generate_vertices() const
   {
      const int segments = 60;
      std::vector<meddl::engine::Vertex> vertices;
      vertices.reserve(segments * 3);

      for (int i = 0; i < segments; i++) {
         float angle1 = 2.0f * M_PI * i / segments;
         float angle2 = 2.0f * M_PI * (i + 1) / segments;

         vertices.push_back({.position = position,
                             .color = color,
                             .normal = {0.0f, 0.0f, 1.0f},
                             .texCoord = {0.5f, 0.5f}});

         vertices.push_back(
             {.position = {position.x + radius * std::cos(angle1),
                           position.y + radius * std::sin(angle1),
                           position.z},
              .color = color,
              .normal = {0.0f, 0.0f, 1.0f},
              .texCoord = {(std::cos(angle1) + 1.0f) / 2.0f, (std::sin(angle1) + 1.0f) / 2.0f}});

         vertices.push_back(
             {.position = {position.x + radius * std::cos(angle2),
                           position.y + radius * std::sin(angle2),
                           position.z},
              .color = color,
              .normal = {0.0f, 0.0f, 1.0f},
              .texCoord = {(std::cos(angle2) + 1.0f) / 2.0f, (std::sin(angle2) + 1.0f) / 2.0f}});
      }

      return vertices;
   }

   [[nodiscard]] std::vector<uint32_t> generate_indices() const
   {
      const int segments = 60;
      std::vector<uint32_t> indices;
      indices.reserve(segments * 3);
      for (int i = 0; i < segments; i++) {
         uint32_t base_idx = i * 3;
         indices.push_back(base_idx);
         indices.push_back(base_idx + 1);
         indices.push_back(base_idx + 2);
      }
      return indices;
   }

   void update()
   {
      // Store previous position to handle collision properly
      glm::vec3 previousPosition = position;
      position += velocity;

      // bounce off the edges
      if (position.x + radius > 1.0f) {
         position.x = 1.0f - radius;  // Place ball exactly at the edge
         velocity.x = -velocity.x;
      }
      else if (position.x - radius < -1.0f) {
         position.x = -1.0f + radius;  // Place ball exactly at the edge
         velocity.x = -velocity.x;
      }

      if (position.y + radius > 1.0f) {
         position.y = 1.0f - radius;  // Place ball exactly at the edge
         velocity.y = -velocity.y;
      }
      else if (position.y - radius < -1.0f) {
         position.y = -1.0f + radius;  // Place ball exactly at the edge
         velocity.y = -velocity.y;
      }
   }
};

int main()
{
   using namespace meddl::render;
   using namespace meddl::events;
   auto renderer =
       RendererBuilder(RendererBuilder::Platform::Glfw, RendererBuilder::Backend::Vulkan)
           .window_size(480, 480)
           .build();

   Ball ball;
   EventHandler handler;

   // Camera parameters
   float camera_distance = 3.0f;
   float camera_rotation_x = 0.4f;
   float camera_rotation_y = 0.7f;
   float model_scale = 1.0f;

   ModelLoader model_loader;
   std::vector<meddl::engine::Vertex> vertices;
   std::vector<uint32_t> indices;

   if (model_loader.load("ler.glb", model_scale)) {
      vertices = model_loader.vertices();
      indices = model_loader.indices();
      meddl::log::info("Model loaded successfully with {} vertices and {} indices",
                       vertices.size(),
                       indices.size());
   }
   else {
      meddl::log::info("Load failed, GG");
      std::exit(0);
   }

   KeyPressed quitter{.keycode = Key::Q};
   handler.subscribe(quitter, [](const Event&) {
      std::exit(0);
      return true;
   });

   // KeyPressed size_inc{.keycode = Key::Space};
   // KeyPressed size_dec{.keycode = Key::Space, .modifiers = KeyModifier::Shift};
   // handler.subscribe(size_inc, [&ball](const Event&) {
   //    ball.radius += 0.1;
   //    return true;
   // });
   // handler.subscribe(size_dec, [&ball](const Event&) {
   //    ball.radius -= 0.1;
   //    return true;
   // });
   // Camera controls
   KeyPressed zoom_in{.keycode = Key::W};
   KeyPressed zoom_out{.keycode = Key::S};
   KeyPressed rotate_left{.keycode = Key::A};
   KeyPressed rotate_right{.keycode = Key::D};
   KeyPressed rotate_up{.keycode = Key::Up};
   KeyPressed rotate_down{.keycode = Key::Down};
   KeyPressed scale_up{.keycode = Key::Minus, .modifiers = KeyModifier::Shift};
   KeyPressed scale_down{.keycode = Key::Minus};

   handler.subscribe(zoom_in, [&camera_distance](const Event&) {
      camera_distance = std::max(camera_distance - 0.1f, 0.5f);
      meddl::log::info("zoom in, camera distance: {}", camera_distance);
      return true;
   });
   handler.subscribe(zoom_out, [&camera_distance](const Event&) {
      camera_distance += 0.1f;
      meddl::log::info("zoom out, camera distance: {}", camera_distance);
      return true;
   });
   handler.subscribe(rotate_left, [&camera_rotation_y](const Event&) {
      camera_rotation_y -= 0.1f;
      meddl::log::info("rotate left, camera rotation y: {}", camera_rotation_y);
      return true;
   });
   handler.subscribe(rotate_right, [&camera_rotation_y](const Event&) {
      camera_rotation_y += 0.1f;
      meddl::log::info("rotate right, camera rotation y: {}", camera_rotation_y);
      return true;
   });
   handler.subscribe(rotate_up, [&camera_rotation_x](const Event&) {
      camera_rotation_x = std::max(camera_rotation_x - 0.1f, -1.5f);
      meddl::log::info("rotate up, camera rotation x: {}", camera_rotation_x);
      return true;
   });
   handler.subscribe(rotate_down, [&camera_rotation_x](const Event&) {
      camera_rotation_x = std::min(camera_rotation_x + 0.1f, 1.5f);
      meddl::log::info("rotate down, camera rotation x: {}", camera_rotation_x);
      return true;
   });
   handler.subscribe(scale_up, [&model_scale, &model_loader, &vertices, &indices](const Event&) {
      model_scale *= 1.1f;
      model_loader.load("ler.glb", model_scale);
      vertices = model_loader.vertices();
      indices = model_loader.indices();
      return true;
   });
   handler.subscribe(scale_down, [&model_scale, &model_loader, &vertices, &indices](const Event&) {
      model_scale = std::max(model_scale * 0.9f, 0.1f);
      model_loader.load("ler.glb", model_scale);
      vertices = model_loader.vertices();
      indices = model_loader.indices();
      return true;
   });

   // Update camera transform
   auto update_camera = [&renderer, &camera_distance, &camera_rotation_x, &camera_rotation_y]() {
      // Calculate camera position based on spherical coordinates
      float x = camera_distance * std::sin(camera_rotation_x) * std::cos(camera_rotation_y);
      float y = camera_distance * std::cos(camera_rotation_x);
      float z = camera_distance * std::sin(camera_rotation_x) * std::sin(camera_rotation_y);

      // Create view matrix
      glm::mat4 view = glm::lookAt(glm::vec3(x, y, z),           // Camera position
                                   glm::vec3(0.0f, 0.0f, 0.0f),  // Look at origin
                                   glm::vec3(0.0f, 1.0f, 0.0f)   // Up vector
      );

      // Set view matrix in renderer
      // Note: This requires adding a method to your renderer to set the view matrix
      renderer.set_view_matrix(view);

      // If your renderer doesn't have this method, you'll need to modify it
   };

   renderer.window()->register_event_handler(handler);

   constexpr auto framerate = 16;
   while (true) {
      renderer.window()->poll_events();
      update_camera();
      // ball.update();
      // auto vertices = ball.generate_vertices();
      // auto indices = ball.generate_indices();
      auto indicies = model_loader.indices();
      for (size_t i = 0; i < indicies.size(); i += 3) {
         // Switch from [0,1,2] to [0,2,1] to reverse winding order
         uint32_t temp = indices[i + 1];
         indices[i + 1] = indices[i + 2];
         indices[i + 2] = temp;
      }
      renderer.set_vertices(model_loader.vertices());
      renderer.set_indices(indicies);
      renderer.draw();
      std::this_thread::sleep_for(std::chrono::milliseconds(framerate));
   }
}
