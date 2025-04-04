
#include <chrono>
#include <thread>

#include "GLFW/glfw3.h"
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
   auto model = meddl::loader::load_model("examples/spaceship/spaceship.glb");
   // auto model = meddl::loader::load_model("ler.glb");
   if (model) {
      meddl::log::info("Loaded the spaceship!");
   }
   else {
      meddl::log::error("{}", model.error().full_message());
   }
   std::this_thread::sleep_for(std::chrono::seconds(2));
   return 0;
}
