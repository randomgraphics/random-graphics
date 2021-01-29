#include <rg/vulkan.h>
#include <GLFW/glfw3.h>
#include <memory>

using namespace rg;
using namespace rg::gl;

void render() {
    clearScreen(.0f, .5f, .5f);
}

int main(int argc, char ** argv) {

    // initialize GLFW
    if (!glfwInit()) { RG_LOGE("failed to initialize GLFW3."); return -1; }
    ScopeExit cleanup([]{ glfwTerminate(); });

    // create window
    auto deleter = [](GLFWwindow * w){ if (w) glfwDestroyWindow(w); };
    std::unique_ptr<GLFWwindow, decltype(deleter)> window(glfwCreateWindow(1280, 720, "opengl-test", nullptr, nullptr), deleter);
    
    glfwMakeContextCurrent(window.get());

    if (!initExtensions()) return -1;

    while(!glfwWindowShouldClose(window.get())) {
        render();
        glfwSwapBuffers(window.get());
        glfwPollEvents();
    }

    return 0;
}
