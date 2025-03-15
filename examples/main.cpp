#include <print>
#include <shaderc/shaderc.hpp>
#include <thread>

#include "GLFW/glfw3.h"
#include "engine/renderer.h"
#include "engine/vertex.h"

struct Ball {
   glm::vec3 position{0.0f, 0.0f, 0.0f};
   glm::vec3 velocity{0.01f, 0.008f, 0.0f};
   float radius{0.15f};
   glm::vec3 color{1.0f, 0.3f, 0.2f};

   std::vector<meddl::engine::Vertex> generate_vertices() const
   {
      const int segments = 20;
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

   // Update ball position based on velocity
   void update()
   {
      position += velocity;

      // Bounce off the edges
      if (position.x + radius > 1.0f || position.x - radius < -1.0f) {
         velocity.x = -velocity.x;
      }

      if (position.y + radius > 1.0f || position.y - radius < -1.0f) {
         velocity.y = -velocity.y;
      }
   }
};

auto main() -> int
{
   meddl::Renderer renderer;
   Ball ball;

   constexpr auto framerate = 16;
   while (true) {
      glfwPollEvents();
      ball.update();
      auto vertices = ball.generate_vertices();
      renderer.set_vertices(vertices);
      renderer.draw();
      std::this_thread::sleep_for(std::chrono::milliseconds(framerate));
   }

   return EXIT_SUCCESS;
}
