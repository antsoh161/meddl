#include <iostream>
#include <thread>

#include "core/asserts.h"

#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

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
