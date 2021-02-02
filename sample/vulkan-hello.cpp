#include <rg/vulkan.h>
#include <GLFW/glfw3.h>
#include <sstream>

using namespace rg;
using namespace rg::vk;

struct SimpleVulkanDevice {

    struct VulkanQueue {
        VkQueue  q = 0;
        uint32_t i = (uint32_t)-1;
        VkCommandPool p; // command pool for this queue.
        operator VkQueue() const { return q; }
        bool operator ==(const VulkanQueue & rhs) const { return q == rhs.q; }
        bool operator !=(const VulkanQueue & rhs) const { return q != rhs.q; }
        bool empty() const { return 0 == q; }
        void cleanup() { q = 0; i = (uint32_t)-1; p = 0; }
    };

    VkAllocationCallbacks * allocator = 0; // reserved for future use. must be empty for now.
    VkPhysicalDevice phydev; // no need to release this one.
    VkDevice device = 0;
    std::vector<VkCommandPool> commandPools; // one pool per queue family.
    VulkanQueue graphicsQueue, presentQueue, computeQueue, dmaQueue;

    operator VkDevice() const { return device; }

    bool init(VkInstance instance, VkSurfaceKHR surface, VkAllocationCallbacks * allocator_) {
        cleanup();

        this->allocator = allocator_;

        // enumerate physical devices
        uint32_t count;
        RG_VKCHK(vkEnumeratePhysicalDevices(instance, &count, nullptr), return false);
        std::vector<VkPhysicalDevice> phydevs(count);
        RG_VKCHK(vkEnumeratePhysicalDevices(instance, &count, phydevs.data()), return false);
        std::stringstream ss; ss << "Available Vulkan physical devices:\n";
        for (const auto & d : phydevs) {
            VkPhysicalDeviceProperties p = {};
            vkGetPhysicalDeviceProperties(d, &p);
            ss << "  " << p.deviceName << "\n";
        }
        RG_LOGI(ss);

        // TODO: pick the one specified by user, or the most powerful one.
        phydev = phydevs[0];

        // query queues
        vkGetPhysicalDeviceQueueFamilyProperties(phydev, &count, nullptr);
        std::vector<VkQueueFamilyProperties> queueFamilies;
        queueFamilies.resize(count);
        vkGetPhysicalDeviceQueueFamilyProperties(phydev, &count, queueFamilies.data());
        RG_LOGI("The selected physical device support %d queue families.", count);

        // create device, one queue for each family
        float queuePriority = 1.0f;
        std::vector<VkDeviceQueueCreateInfo> queueCreateInfo;
        for(uint32_t i = 0; i < queueFamilies.size(); ++i) {
            queueCreateInfo.push_back(VkDeviceQueueCreateInfo{
                VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO, nullptr, 0,
                i,
                1,
                &queuePriority
            });
        }
        VkPhysicalDeviceFeatures deviceFeatures = {};
        std::vector<const char *> deviceExtensions;
        if (surface) deviceExtensions.push_back(VK_KHR_SWAPCHAIN_EXTENSION_NAME);
        VkDeviceCreateInfo deviceCreateInfo = {
            VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO, nullptr, 0,
            (uint32_t)queueCreateInfo.size(), queueCreateInfo.data(),
            0, nullptr,
            (uint32_t)deviceExtensions.size(), deviceExtensions.data(),
            &deviceFeatures,
        };
        RG_VKCHK(vkCreateDevice(phydev, &deviceCreateInfo, allocator, &device), return false);

        // go through queue families
        for(uint32_t i = 0; i < queueFamilies.size(); ++i) {
            const auto & f = queueFamilies[i];

            // create a default command pool per family
            auto poolCI = VkCommandPoolCreateInfo {
                VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO, nullptr,
                VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT, // allow individual reset of each command buffer.
                i,
            };
            VkCommandPool pool = 0;
            RG_VKCHK(vkCreateCommandPool(device, &poolCI, allocator, &pool), return false);
            commandPools.push_back(pool);

            // classify all queues
            if (!graphicsQueue && f.queueFlags & VK_QUEUE_GRAPHICS_BIT) {
                vkGetDeviceQueue(device, i, 0, &graphicsQueue.q);
                graphicsQueue.i = i;
                graphicsQueue.p = pool;
            }
            // if (!computeQueue && f.queueFlags & VK_QUEUE_COMPUTE_BIT) {
            //     vkGetDeviceQueue(device, i, 0, &computeQueue.q);
            //     computeQueue.i = i;
            //     computeQueue.p = pool;
            // }
            // if (!dmaQueue && f.queueFlags & VK_QUEUE_TRANSFER_BIT) {
            //     vkGetDeviceQueue(device, i, 0, &dmaQueue.q);
            //     dmaQueue.i = i;
            //     dmaQueue.p = pool;
            // }
            if (!presentQueue && surface) { // only check for present queue when there's a valid surface to present to.
                VkBool32 presentSupport;
                vkGetPhysicalDeviceSurfaceSupportKHR(phydev, i, surface, &presentSupport);
                if (presentSupport) {
                    vkGetDeviceQueue(device, i, 0, &presentQueue.q);
                    presentQueue.i = i;
                    presentQueue.p = pool;
                }
            }
        }

        // done
        return true;
    }

    void cleanup() {
        graphicsQueue.cleanup();
        computeQueue.cleanup();
        dmaQueue.cleanup();
        presentQueue.cleanup();
        for(auto p : commandPools) {
            vkDestroyCommandPool(device, p, allocator);
        }
        commandPools.clear();
        if (device) vkDestroyDevice(device, allocator), device = 0;
    }
};

struct SimpleVulkanSwapChain {

    struct InitParameters {
        VkInstance                  instance = 0;
        VkAllocationCallbacks *     allocator = 0;
        const SimpleVulkanDevice *  device = nullptr;
        VkSurfaceKHR                surface = 0;
        uint32_t                    width = 0, height = 0;
    };

    struct FrameSync {
        VkDevice                        device = 0;
        VkAllocationCallbacks *         allocator = 0;
        VkSemaphore *                   semaphores = 0;
        VkFence *                       fences = 0;
        size_t                          count = 0;
        uint32_t                        current = 0;

        bool init(VkDevice d, VkAllocationCallbacks * a, size_t c) {
            device = d;
            allocator = a;
            semaphores = new VkSemaphore[c];
            fences = new VkFence[c];
            count = c;
            current = 0;
            for(size_t i = 0; i < count; ++i) {
                VkSemaphoreCreateInfo sci = { VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO };
                RG_VKCHK(vkCreateSemaphore(device, &sci, allocator, &semaphores[i]), return false);
                VkFenceCreateInfo fci = { VK_STRUCTURE_TYPE_FENCE_CREATE_INFO };
                RG_VKCHK(vkCreateFence(device, &fci, allocator, &fences[i]), return false);
            }
            return true;
        }

        void cleanup() {
            for(uint32_t i = 0; i < count; ++i) {
                vkDestroySemaphore(device, semaphores[i], allocator);
                vkDestroyFence(device, fences[i], allocator);
            }
            count = 0;
            if (semaphores) delete [] semaphores, semaphores = 0;
            if (fences) delete[] fences, fences = 0;
        }

        void moveToNext() {
            current = (current + 1) % count;
        }

        operator VkSemaphore () const { return semaphores[current]; }
        operator VkFence () const { return fences[current]; }
    };

    struct BackBuffer {
        VkDevice                device = 0;
        VkAllocationCallbacks * allocator = 0;
        VkFormat                colorFormat = VK_FORMAT_UNDEFINED;
        VkImage                 colorImage = 0;
        VkImageView             colorView  = 0;
        VkFormat                dsFormat = VK_FORMAT_UNDEFINED;
        VkImage                 dsImage = 0;
        VkImage                 depthView = 0;
        VkImage                 stencilView = 0;
        VkSemaphore             presentingFinished = 0; ///< indicating that presenting of the back buffer is done that is ready for GPU to write to it.
        VkSemaphore             renderingFinished = 0; ///< indicating that GPU process on the back buffer is done that is is ready for presenting.

        void cleanup() {
            if (renderingFinished) vkDestroySemaphore(device, renderingFinished, allocator), renderingFinished = 0;
            if (colorView) vkDestroyImageView(device, colorView, allocator), colorView = 0;
            colorImage = 0; // the image is retreived from swapchian object. no need to delete them explicitly.
        }
    };

    InitParameters          initParameters = {};
    VkSwapchainKHR          swapchain = 0;
    std::vector<BackBuffer> backbuffers;
    uint32_t                activeBackBufferIndex = 0; // index of the back buffer for current frame's rendering.
    FrameSync               presentingFinished;

    operator VkSwapchainKHR() const { return swapchain; }

    BackBuffer & activeBackBuffer() { return backbuffers[activeBackBufferIndex]; }

    bool init(const InitParameters & ip) {

        cleanup();

        initParameters = ip;

        VkSurfaceCapabilitiesKHR surfaceCaps = {};
        vkGetPhysicalDeviceSurfaceCapabilitiesKHR(ip.device->phydev, ip.surface, &surfaceCaps);
        std::vector<uint32_t> queueIndices;
        if (ip.device->graphicsQueue != ip.device->presentQueue) {
            queueIndices.push_back(ip.device->graphicsQueue.i);
            queueIndices.push_back(ip.device->presentQueue.i);
        }
        VkSwapchainCreateInfoKHR swapChainCreateInfo = {
            VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR, nullptr, 0,
            ip.surface,
            2, // double buffer
            VK_FORMAT_B8G8R8A8_UNORM, VK_COLOR_SPACE_SRGB_NONLINEAR_KHR,
            { ip.width, ip.height },
            1, // image array layers (2 for stereoscopic rendering)
            VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT,
            ip.device->graphicsQueue == ip.device->presentQueue ? VK_SHARING_MODE_EXCLUSIVE : VK_SHARING_MODE_CONCURRENT,
            (uint32_t)queueIndices.size(), queueIndices.data(),
            surfaceCaps.currentTransform,
            VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR,
            VK_PRESENT_MODE_FIFO_KHR, // mailbox?
            true, // clipped.
            0, // old swap chain
        };
        RG_VKCHK(vkCreateSwapchainKHR(ip.device->device, &swapChainCreateInfo, ip.allocator, &swapchain), return false);

        // acquire back buffers
        uint32_t count;
        std::vector<VkImage> swapchainImages;
        vkGetSwapchainImagesKHR(ip.device->device, swapchain, &count, nullptr);
        swapchainImages.resize(count);
        vkGetSwapchainImagesKHR(ip.device->device, swapchain, &count, swapchainImages.data());
        backbuffers.resize(swapchainImages.size());
        for (size_t i = 0; i < swapchainImages.size(); i++) {

            auto & bb = backbuffers[i];

            bb.device = ip.device->device;
            bb.allocator = ip.allocator;
            bb.colorFormat = swapChainCreateInfo.imageFormat;
            bb.colorImage = swapchainImages[i];

            VkImageViewCreateInfo createInfo = {};
            createInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
            createInfo.image = swapchainImages[i];
            createInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
            createInfo.format = swapChainCreateInfo.imageFormat;
            createInfo.components.r = VK_COMPONENT_SWIZZLE_IDENTITY;
            createInfo.components.g = VK_COMPONENT_SWIZZLE_IDENTITY;
            createInfo.components.b = VK_COMPONENT_SWIZZLE_IDENTITY;
            createInfo.components.a = VK_COMPONENT_SWIZZLE_IDENTITY;
            createInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
            createInfo.subresourceRange.baseMipLevel = 0;
            createInfo.subresourceRange.levelCount = 1;
            createInfo.subresourceRange.baseArrayLayer = 0;
            createInfo.subresourceRange.layerCount = 1;
            RG_VKCHK(vkCreateImageView(ip.device->device, &createInfo, ip.allocator, &bb.colorView), return false);

            VkSemaphoreCreateInfo sci = { VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO };
            RG_VKCHK(vkCreateSemaphore(ip.device->device, &sci, ip.allocator, &bb.renderingFinished), return false);
        }

        // create frame throttling object
        if (!presentingFinished.init(ip.device->device, ip.allocator, swapchainImages.size())) return false;

        // acquire a back buffer for the incoming rendering commands
        RG_VKCHK(acquireNextBackBuffer(), return false);

        // done
        return true;
    }

    void cleanup() {
        presentingFinished.cleanup();
        for(auto & b : backbuffers) {
            b.cleanup();
        }
        backbuffers.clear();
        if (swapchain) vkDestroySwapchainKHR(initParameters.device->device, swapchain, initParameters.allocator), swapchain = 0;
    }

    VkResult acquireNextBackBuffer() {
        auto vk = vkAcquireNextImageKHR(initParameters.device->device, swapchain, (uint64_t)-1, presentingFinished, VK_NULL_HANDLE, &activeBackBufferIndex);
        RG_VKCHK(vk, return vk);
        backbuffers[activeBackBufferIndex].presentingFinished = presentingFinished;
        presentingFinished.moveToNext();
        return vk;
    }

    VkResult present() {
        VkPresentInfoKHR presentInfo = {};
        presentInfo.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
        presentInfo.waitSemaphoreCount = 1;
        presentInfo.pWaitSemaphores = &backbuffers[activeBackBufferIndex].renderingFinished;
        presentInfo.swapchainCount = 1;
        presentInfo.pSwapchains = &swapchain;
        presentInfo.pImageIndices = &activeBackBufferIndex;
        presentInfo.pResults = nullptr; // Optional
        auto vk = vkQueuePresentKHR(initParameters.device->presentQueue, &presentInfo);
        RG_VKCHK(vk, return vk);
        RG_VKCHK(vk = acquireNextBackBuffer(),);
        return vk;
    }
};

/// structure to describe simple render pass with only 1 subpass
struct SimpleRendePassCreationParameters {
    VkDevice                        device = 0;
    VkAllocationCallbacks *         allocator = 0;
    uint32_t                        samples = 1; ///< numer of samples
    std::initializer_list<VkFormat> colors; ///< color attatchment formats, one per color attatchment.
    VkFormat                        depthstencil = VK_FORMAT_UNDEFINED; ///< depth stencil format. VK_FORMAT_UNDEFINED means no depth
    bool                            present = false; ///< set to true when you are about to present the color attatchment 
};

/// create simple render pass with only 1 subpass
VkRenderPass createSimpleRenderPass(const SimpleRendePassCreationParameters & cp) {
    std::vector<VkAttachmentDescription> attachments;
    std::vector<VkAttachmentReference> refs;

    // color attachments
    for(auto c : cp.colors) {
        VkAttachmentDescription a = {};
        a.format = c;
        a.samples = (VkSampleCountFlagBits)cp.samples;
        a.loadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
        a.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
        a.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
        a.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
        a.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
        a.finalLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
        attachments.push_back(a);
        refs.push_back({
            (uint32_t)attachments.size() - 1,
            VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL
        });
    }
    
    if (cp.present) {
        attachments[0].finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;
    }

    // depth stencil attachment
    if (VK_FORMAT_UNDEFINED != cp.depthstencil) {
        VkAttachmentDescription a = {};
        a.format = cp.depthstencil;
        a.samples = (VkSampleCountFlagBits)cp.samples;
        a.loadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
        a.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
        a.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
        a.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
        a.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
        a.finalLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
        attachments.push_back(a);
        refs.push_back({
            (uint32_t)attachments.size() - 1,
            VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL
        });
    }

    VkSubpassDescription subpass = {};
    subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
    subpass.colorAttachmentCount = (uint32_t)cp.colors.size();
    subpass.pColorAttachments = refs.data();

    VkSubpassDependency dependency = {};
    dependency.srcSubpass = VK_SUBPASS_EXTERNAL;
    dependency.srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
    dependency.srcAccessMask = 0;
    dependency.dstSubpass = 0;
    dependency.dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
    dependency.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_READ_BIT | VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;

    VkRenderPassCreateInfo renderPassInfo = {};
    renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
    renderPassInfo.attachmentCount = (uint32_t)attachments.size();
    renderPassInfo.pAttachments = attachments.data();
    renderPassInfo.subpassCount = 1;
    renderPassInfo.pSubpasses = &subpass;
    renderPassInfo.dependencyCount = 1;
    renderPassInfo.pDependencies = &dependency;
    VkRenderPass renderpass;
    RG_VKCHK(vkCreateRenderPass(cp.device, &renderPassInfo, cp.allocator, &renderpass), return 0);

    // done
    return renderpass;
};

/// Represents a collection of images (color/depth/stencil) that a render pass can render to. 
/// It can either point to a back buffer, or some other offscreen image.
/// When it points to back buffer, we'll need to create one for each back buffer.
struct SimpleVulkanFrameBuffer {

    struct InitParameters {
        const SimpleVulkanSwapChain::BackBuffer * backbuffer = nullptr;
        std::initializer_list<VkFormat> colorFormats;
        VkFormat dsFormat = VK_FORMAT_UNDEFINED;
    };

    bool init(const InitParameters &) {
        cleanup();
    }

    void cleanup() {

    }
};

/// This is the primary structure that we can use for rendering. It contains:
///     - One frame buffer object to store the render result.
///     - One VkRenderPass instance
///     - One command buffer object that we can record command to
struct SimpleVulkanRenderPass {

    struct InitParameters {
        VkDevice                  device;
        SimpleVulkanFrameBuffer * frameBuffer;
        VkQueue                   queue;
        VkCommandPool             cp;
    };

    InitParameters  initParameters = {};
    VkCommandBuffer cb = 0;
    VkFence         completionFence = 0; // the fence object that marks the completion of the render pass.
    VkSemaphore     completionSemaphore = 0;

    bool init(const InitParameters & ip) {
        cleanup();
        
        initParameters = ip;

        // done
        return true;
    }

    void cleanup() {
    }

    void submit(const std::initializer_list<VkSemaphore> & waits) {
        VkPipelineStageFlags stages = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT; // make this an init parameters.
        VkSubmitInfo si = {};
        si.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
        si.waitSemaphoreCount = (uint32_t)waits.size();
        si.pWaitSemaphores = waits.begin();
        si.pWaitDstStageMask = &stages;
        si.commandBufferCount = 1;
        si.pCommandBuffers = &cb;
        si.signalSemaphoreCount = 1;
        si.pSignalSemaphores = &completionSemaphore;
        vkQueueSubmit(initParameters.queue, 1, &si, completionFence);
    }
};

struct SimpleVulkanApp {
    VkAllocationCallbacks * allocator = 0; // reserved for future use. must be empty for now.
    VkInstance instance = 0;
    VkDebugReportCallbackEXT debugReport = 0;
    VkSurfaceKHR surface = 0;
    SimpleVulkanDevice device = {};
    SimpleVulkanSwapChain swapchain = {};
    VkRenderPass finalRenderPass = 0; // the final render pass in the frame to render to back buffer.

    ~SimpleVulkanApp() { cleanup(); }

    bool init(GLFWwindow * window) {

        cleanup();

        RG_VKCHK(volkInitialize(), return false);

        // create VK instance
        const char* layers[] = {
            "VK_LAYER_KHRONOS_validation", // TODO: query name of the actual debug layer on Android
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

        // initialize extensions
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

        // create surface
        RG_VKCHK(glfwCreateWindowSurface(instance, window, allocator, &surface), return false);

        // create device
        if (!device.init(instance, surface, allocator)) return false;

        // create swap chain
        uint32_t width, height;
        glfwGetWindowSize(window, (int*)&width, (int*)&height);
        if (!swapchain.init({instance, allocator, &device, surface, width, height})) return false;

        // done
        return true;
    }

    void present() {
        // TODO: recreate swapchian when window size changed.
        swapchain.present();
    }

    void cleanup() {
        if (finalRenderPass) vkDestroyRenderPass(device, finalRenderPass, allocator), finalRenderPass = 0;
        swapchain.cleanup();
        device.cleanup();
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

void render() {
}

int main(int argc, char ** argv) {
    (void)argc;
    (void)argv;

    // initialize GLFW
    if (!glfwInit()) { RG_LOGE("failed to initialize GLFW3."); return -1; }
    auto glfwCleanup = ScopeExit([]{ glfwTerminate(); });

    // create an window
    glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
    auto window = glfwCreateWindow(1280, 720, "opengl-test", nullptr, nullptr);
    auto cleanupWindow = ScopeExit([&]{ glfwDestroyWindow(window); });

    // initialize Vulkan app
    SimpleVulkanApp app;
    if (!app.init(window)) return -1;

    // // create frame buffers
    // std::vector<SimpleVulkanFrameBuffer> frameBuffers(app.swapchain.backbuffers.size());
    // for(size_t i = 0; i < app.swapchain.backbuffers.size(); ++i) {
    //     if (!frameBuffers[i].init({
    //         {&app.swapchain.backbuffers[i]},
    //     })) return false;
    // }

    // // create main render pass
    // auto rp = createSimpleRenderPass({
    //     app.device,
    //     app.allocator,
    //     1, // 1 samples
    //     { app.swapchain.backbuffers[0].colorFormat },
    //     VK_FORMAT_UNDEFINED, // no depth/stencil
    //     true, // present
    // });
    // if (!rp) return false;

    // allocate command buffer
    auto cbai = VkCommandBufferAllocateInfo {
        VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO, nullptr,
        app.device.graphicsQueue.p,
        VK_COMMAND_BUFFER_LEVEL_PRIMARY,
        1,
    };
    VkCommandBuffer cb;
    RG_VKCHK(vkAllocateCommandBuffers(app.device, &cbai, &cb), return -1);

    while(!glfwWindowShouldClose(window)) {

        // record the command buffer
        auto cbbi = VkCommandBufferBeginInfo { VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO, nullptr, VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT };
        RG_VKCHK(vkBeginCommandBuffer(cb, &cbbi), return -1);

        // change layout for clearing
        auto barrier = VkImageMemoryBarrier { VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER };
        barrier.image = app.swapchain.activeBackBuffer().colorImage;
        barrier.oldLayout = VK_IMAGE_LAYOUT_UNDEFINED;
        barrier.newLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
        barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
        barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
        barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        barrier.subresourceRange.baseMipLevel = 0;
        barrier.subresourceRange.levelCount = 1;
        barrier.subresourceRange.baseArrayLayer = 0;
        barrier.subresourceRange.layerCount = 1;
        vkCmdPipelineBarrier(
            cb,
            VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT, VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT,
            0,    // dependency flags
            0, 0, // memory barriers
            0, 0, // buffer barries
            1, &barrier);

        // clear 
        auto clearColor = VkClearColorValue{{1.f, .0f, .0f, 1.f }}; // clear to RED
        auto clearRange = VkImageSubresourceRange { VK_IMAGE_ASPECT_COLOR_BIT, 0, 1, 0, 1 }; // clear base map only.
        vkCmdClearColorImage(cb, app.swapchain.activeBackBuffer().colorImage, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, &clearColor, 1, &clearRange);

        // change layout back for presentation
        barrier.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
        barrier.newLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;
        vkCmdPipelineBarrier(
            cb,
            VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT, VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT,
            0,    // dependency flags
            0, 0, // memory barriers
            0, 0, // buffer barries
            1, &barrier);

        // end of recording
        RG_VKCHK(vkEndCommandBuffer(cb), return -1);

        // submit the command buffer
        VkPipelineStageFlags psf = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
        auto si = VkSubmitInfo {
            VK_STRUCTURE_TYPE_SUBMIT_INFO, nullptr,
            1, &app.swapchain.activeBackBuffer().presentingFinished, &psf, // wait for presenting on this back buffer to finish
            1, &cb,
            1, &app.swapchain.activeBackBuffer().renderingFinished, // signal this semaphore when rendering is done.
        };
        vkQueueSubmit(app.device.graphicsQueue.q, 1, &si, VK_NULL_HANDLE);

        // TODO: better throttling
        vkQueueWaitIdle(app.device.graphicsQueue);
        
        app.present();
        glfwPollEvents();
    }

    return 0;
}
