#include "core/asserts.h"
#include "core/engine.h"

int main() {
    Engine engine;
    try {
        engine.init_glfw();
        engine.init_vulkan();
    } catch (const std::exception& e) {
        M_ASSERT_U("Initialization caught exception: {}", e.what())
    }
    engine.run();
    return EXIT_SUCCESS;
}
