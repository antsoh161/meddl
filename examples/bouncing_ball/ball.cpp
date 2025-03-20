
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
#include "spdlog/common.h"

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
       RendererBuilder(RendererBuilder::Platform::Glfw, RendererBuilder::Backend::Vulkan).build();
   Ball ball;
   EventHandler handler;

   KeyPressed quitter{.keycode = Key::Q};
   handler.subscribe(quitter, [](const Event&) {
      std::exit(0);
      return true;
   });

   KeyPressed size_inc{.keycode = Key::Space};
   KeyPressed size_dec{.keycode = Key::Space, .modifiers = KeyModifier::Shift};
   handler.subscribe(size_inc, [&ball](const Event&){
      ball.radius += 0.1;
      return true;
   });
   handler.subscribe(size_dec, [&ball](const Event&){
      ball.radius -= 0.1;
      return true;
   });


   renderer.window()->register_event_handler(handler);

   constexpr auto framerate = 8;
   while (true) {
      renderer.window()->poll_events();
      ball.update();
      auto vertices = ball.generate_vertices();
      auto indices = ball.generate_indices();
      renderer.set_vertices(vertices);
      renderer.set_indices(indices);
      renderer.draw();
      std::this_thread::sleep_for(std::chrono::milliseconds(framerate));
   }
}
