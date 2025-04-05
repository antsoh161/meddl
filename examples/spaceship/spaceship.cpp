
#include <chrono>
#include <thread>

#include "GLFW/glfw3.h"
#include "engine/events/event.h"
#include "engine/loader.h"
#include "engine/renderer.h"

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

int main()
{
   using namespace meddl::render;
   using namespace meddl::events;
   auto renderer =
       RendererBuilder(RendererBuilder::Platform::Glfw, RendererBuilder::Backend::Vulkan)
           .window_size(480, 480)
           .build();

   EventHandler handler;

   KeyPressed quit = {.keycode = Key::Q};
   renderer.window()->register_event_handler(handler);
   handler.subscribe(quit, [](const Event&) {
      std::exit(0);
      return true;
   });

   auto model = meddl::loader::load_model("examples/spaceship/spaceship.glb");
   if (model) {
      meddl::log::info("Loaded the spaceship!");
   }
   else {
      meddl::log::error("{}", model.error().full_message());
   }
   std::vector<meddl::Vertex> all_vertices{};
   std::vector<uint32_t> all_indices{};
   for (const auto& mesh : model->meshes) {
      auto vertex_offset = static_cast<uint32_t>(all_vertices.size());
      all_vertices.insert(all_vertices.end(), mesh.vertices.begin(), mesh.vertices.end());
      for (uint32_t index : mesh.indices) {
         all_indices.push_back(vertex_offset + index);
      }
   }

   glm::mat4 view = glm::lookAt(glm::vec3(0.0f, 0.5f, 2.0f),  // Camera position
                                glm::vec3(0.0f, 0.0f, 0.0f),  // Look at target
                                glm::vec3(0.0f, 1.0f, 0.0f)   // Up vector
   );
   renderer.set_view_matrix(view);
   constexpr auto framerate = 16;
   uint64_t counter{0};
   renderer.set_textures(*model);
   while (true) {
      renderer.window()->poll_events();
      renderer.set_vertices(all_vertices);
      renderer.set_indices(all_indices);
      renderer.draw();
      std::this_thread::sleep_for(std::chrono::milliseconds(framerate));
      counter++;
   }
   return 0;
}
