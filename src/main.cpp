#include "demo.h"
#include "platform.h"

#include "glfw/glfw3.h"

#include <cassert>

static int window_width = 720;
static int window_height = 720;

static void glfw_error_callback(int error, const char* description) {
    fprintf(stderr, "GLFW error: %s\n", description);
}

int main(int argc, char** argv) {
    glfwSetErrorCallback(glfw_error_callback);
    if (!glfwInit())
        error("glfwInit failed");

    glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
    GLFWwindow* glfw_window = glfwCreateWindow(window_width, window_height, "Vulkan demo", nullptr, nullptr);
    assert(glfw_window != nullptr);

    Vk_Demo demo{};
    demo.initialize(glfw_window, true);

    bool window_active = true;

    while (!glfwWindowShouldClose(glfw_window)) {
        if (window_active)
            demo.run_frame();

        glfwPollEvents();

        int width, height;
        glfwGetWindowSize(glfw_window, &width, &height);

        window_active = (width != 0 && height != 0);

        if (!window_active)
            continue; 

        platform::sleep(1);
    }

    demo.shutdown();
    glfwTerminate();
    return 0;
}
