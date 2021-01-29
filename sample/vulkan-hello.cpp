#include <rg/vulkan.h>
#include <GLFW/glfw3.h>

using namespace rg;

struct VulkanSwapChain {

};

struct SimpleVulkanApp {
    VkAllocationCallbacks * allocator = 0; // reserved for future use. must be empty for now.
    VkInstance instance = 0;
    VkDebugReportCallbackEXT debugReport = 0;
    VkSurfaceKHR surface = 0;
    VkPhysicalDevice physicalDevice; // no need to release this one.
    VkDevice device = 0;
    VkCommandPool commandPool = 0;

    ~SimpleVulkanApp() { cleanup(); }

    bool init() {

        RG_VKCHK(volkInitialize(), return false);

        // create VK instance
        const char* layers[] = {
            "VK_LAYER_LUNARG_standard_validation", // TODO: query name of the actual debug layer on Android
        };
        const char * extensions[] = {
            VK_EXT_DEBUG_REPORT_EXTENSION_NAME,
            VK_KHR_SURFACE_EXTENSION_NAME,
#if RG_MSWIN            
            VK_KHR_WIN32_SURFACE_EXTENSION_NAME,
#endif
        };
        auto ici = VkInstanceCreateInfo {
            VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO,
            nullptr, // next
            0, // flags,
            nullptr, // app info
            (uint32_t)std::size(layers), layers,
            (uint32_t)std::size(extensions), extensions,
        };
        RG_VKCHK(vkCreateInstance(&ici, allocator, &instance), return false);

        // load extensions
        volkLoadInstance(instance);

        // setup debug callback
        auto debugci = VkDebugReportCallbackCreateInfoEXT{
            VK_STRUCTURE_TYPE_DEBUG_REPORT_CALLBACK_CREATE_INFO_EXT,
            nullptr,
            VK_DEBUG_REPORT_ERROR_BIT_EXT | VK_DEBUG_REPORT_WARNING_BIT_EXT,
            debugCallback,
            this,
        };
        RG_VKCHK(vkCreateDebugReportCallbackEXT(instance, &debugci, allocator, &debugReport), return false);

        // TODO: create swap chain, command queue, command pool, command buffers and etc.

        // done
        return true;
    }

    void present() {
        // TODO: update the swap chain.
    }

    void cleanup() {
        if (commandPool) vkDestroyCommandPool(device, commandPool, allocator), commandPool = 0;
        if (device) vkDestroyDevice(device, allocator), device = 0;
        if (surface) vkDestroySurfaceKHR(instance, surface, allocator), surface = 0;
        if (debugReport) vkDestroyDebugReportCallbackEXT(instance, debugReport, allocator), debugReport = 0;
        if (instance) vkDestroyInstance(instance, allocator), instance = 0;
    }

    static VkBool32 VKAPI_PTR debugCallback(
        VkDebugReportFlagsEXT, //                       flags,
        VkDebugReportObjectTypeEXT, //                  objectType,
        uint64_t, //                                    object,
        size_t, //                                      location,
        int32_t, //                                     messageCode,
        const char* prefix,
        const char* message,
        void*       userData) {
        (void)userData;
        RG_LOGE("[vulkan] %s : %s", prefix, message);
        return VK_FALSE;
    }
};

void render(SimpleVulkanApp &) {
}

int main() {

    // initialize GLFW
    if (!glfwInit()) { RG_LOGE("failed to initialize GLFW3."); return -1; }
    auto glfwCleanup = ScopeExit([]{ glfwTerminate(); });

    // create an window
    glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
    auto window = glfwCreateWindow(1280, 720, "opengl-test", nullptr, nullptr);
    auto cleanupWindow = ScopeExit([&]{ glfwDestroyWindow(window); });

    // initialize Vulkan app
    SimpleVulkanApp app;
    if (!app.init()) return -1;

    while(!glfwWindowShouldClose(window)) {
        render(app);
        app.present();
        glfwPollEvents();
    }

    return 0;
}
