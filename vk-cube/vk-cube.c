// ================================================================================================
// This is a basic spinning cube that I wrote to learn Vulkan.
//
// This program could be structured better. I intentionally kept all the Vulkan API calls in the
// main function so they can be read sequentially. It would be better to create helper functions
// for swapchain creation, memory allocation, etc.
//
// ref: https://docs.vulkan.org
// ref: https://github.com/KhronosGroup/Vulkan-Samples
//
// Changelog:
//     5/31/2026: Initial release
//
// License:
//     Copyright (c) 2026 Hunter Kvalevog
//
//     Permission to use, copy, modify, and/or distribute this software for any
//     purpose with or without fee is hereby granted.
//
//     THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES
//     WITH REGARD TO THIS SOFTWARE.
// ================================================================================================

#include <SDL3/SDL.h>
#include <SDL3/SDL_vulkan.h>
#include <vulkan/vulkan.h>

#include <assert.h>
#include <stdio.h>
#include <stdlib.h>

#if defined(_WIN32)
#   include <windows.h>
#endif

#if defined(__APPLE__) || defined(__linux__)
#   include <unistd.h>
#endif

#ifdef __APPLE__
#   include <vulkan/vulkan_metal.h>
#endif

// ================================================================================================
// Utility code
// ================================================================================================

#define ASSERT(X)    assert(X)
#define COUNTOF(ARR) (sizeof(ARR) / sizeof((ARR)[0]))
#define DEG2RAD(DEG) ((DEG) * 3.14159265f / 180.0f)
#define MAX(A, B)    ((A) > (B) ? (A) : (B))
#define MIN(A, B)    ((A) < (B) ? (A) : (B))
#define UNUSED(X)    ((void)(X))

// Find the index of the appropriate memory type
static uint32_t find_mem_type(VkPhysicalDevice pdev, uint32_t filter, VkMemoryPropertyFlags flags)
{
    VkPhysicalDeviceMemoryProperties mem;
    vkGetPhysicalDeviceMemoryProperties(pdev, &mem);
    for (uint32_t i = 0; i < mem.memoryTypeCount; i++) {
        if ((filter & (1 << i)) && (mem.memoryTypes[i].propertyFlags & flags) == flags) {
            return i;
        }
    }
    assert(0 && "failed to find memory type");
    return 0;
}

// 4x4 identity matrix
static inline void mat4ident(float dst[16])
{
    dst[ 0] = 1.0f; dst[ 1] = 0.0f; dst[ 2] = 0.0f; dst[ 3] = 0.0f;
    dst[ 4] = 0.0f; dst[ 5] = 1.0f; dst[ 6] = 0.0f; dst[ 7] = 0.0f;
    dst[ 8] = 0.0f; dst[ 9] = 0.0f; dst[10] = 1.0f; dst[11] = 0.0f;
    dst[12] = 0.0f; dst[13] = 0.0f; dst[14] = 0.0f; dst[15] = 1.0f;
}

// 4x4 X rotation matrix
static inline void mat4rotx(float dst[16], float rad)
{
    mat4ident(dst);
    dst[ 5] =  SDL_cosf(rad);
    dst[ 9] = -SDL_sinf(rad);
    dst[ 6] =  SDL_sinf(rad);
    dst[10] =  SDL_cosf(rad);
}

// 4x4 Y rotation matrix
static inline void mat4roty(float dst[16], float rad)
{
    mat4ident(dst);
    dst[ 0] =  SDL_cosf(rad);
    dst[ 8] =  SDL_sinf(rad);
    dst[ 2] = -SDL_sinf(rad);
    dst[10] =  SDL_cosf(rad);
}

// 4x4 translation matrix
static inline void mat4translate(float dst[16], float vec[3])
{
    mat4ident(dst);
    dst[12] = vec[0];
    dst[13] = vec[1];
    dst[14] = vec[2];
}

// 4x4 matrix multiplication
static inline void mat4mul(float dst[16], const float left[16], const float right[16])
{
    for (size_t col = 0; col < 4; ++col) {
    for (size_t row = 0; row < 4; ++row) {
        dst[col * 4 + row] =
            left[0 * 4 + row] * right[col * 4 + 0] +
            left[1 * 4 + row] * right[col * 4 + 1] +
            left[2 * 4 + row] * right[col * 4 + 2] +
            left[3 * 4 + row] * right[col * 4 + 3];
    }
    }
}

// 4x4 perspective projection matrix
static inline void mat4perspective(float dst[16], float fov, float aspect, float z0, float z1)
{
    float f = 1.0f / SDL_tanf(fov / 2.0f);
    float nmf = z0 - z1;
    dst[ 0] = f / aspect; dst[ 1] = 0.0f; dst[ 2] = 0.0f;            dst[ 3] = 0.0f;
    dst[ 4] = 0.0f;       dst[ 5] = -f;   dst[ 6] = 0.0f;            dst[ 7] = 0.0f;
    dst[ 8] = 0.0f;       dst[ 9] = 0.0f; dst[10] = z1 / nmf;        dst[11] = -1.0f;
    dst[12] = 0.0f;       dst[13] = 0.0f; dst[14] = (z0 * z1) / nmf; dst[15] = 0.0f;
}

// ================================================================================================
// Application code
// ================================================================================================

int main(int argc, const char **argv)
{
    UNUSED(argc); UNUSED(argv);

    if (!SDL_Init(SDL_INIT_VIDEO)) {
        printf("Failed to initialize SDL: %s", SDL_GetError());
        return 0;
    }

    // Shader binaries should be in the same directory as the demo executable. Reset the working
    // directory to make things reliable.
    {
        const char *exe_dir = SDL_GetBasePath();
        printf("Setting working directory: %s\n", exe_dir);
        // I wish the SDL devs were pragmatic enough to add SDL_SetCurrentDirectory():
        // https://github.com/libsdl-org/SDL/issues/9110
#if defined(_WIN32)
        SetCurrentDirectory(exe_dir);
#endif
#if defined(__APPLE__) || defined(__linux__)
        chdir(exe_dir);
#endif
    }

    // Create VkInstance
    VkInstance vki = 0;
    {
        // Instance extensions are essentially just extensions to the Vulkan spec. Without any
        // extensions, Vulkan can't actually render anything because it doesn't know how to interop
        // with the native OS window.
        uint32_t    num_exts = 0;
        const char *exts[32] = { 0 };
        #define REQUIRE_EXTENSION(NAME) ASSERT(num_exts < COUNTOF(exts)); exts[num_exts++] = NAME;
        
        // SDL has a nice function that tells us what extensions are required for the given video
        // backend.
        uint32_t num_sdl_exts = 0;
        const char *const *sdl_exts = SDL_Vulkan_GetInstanceExtensions(&num_sdl_exts);
        for (uint32_t i = 0; i < num_sdl_exts; ++i) {
            REQUIRE_EXTENSION(sdl_exts[i]);
        }

        // On macOS, we also need to activate the portability extension in order to use MoltenVK.
        // This is currently the only extension we need that isn't mentioned by SDL.
#ifdef __APPLE__
        REQUIRE_EXTENSION(VK_KHR_PORTABILITY_ENUMERATION_EXTENSION_NAME);
#endif

        // Tell the driver about this app. The only thing that relly matters is the API version.
        VkApplicationInfo app_info = {
            .sType      = VK_STRUCTURE_TYPE_APPLICATION_INFO,
            .apiVersion = VK_API_VERSION_1_3,
        };

        // Bitwise flags that change the behavior of the VkInstance. It's basically pointless. The
        // only accepted value in the spec is  VK_INSTANCE_CREATE_ENUMERATE_PORTABILITY_BIT_KHR.
        VkInstanceCreateFlags flags = 0;

        // ...which we need on macOS
#ifdef __APPLE__
        flags |= VK_INSTANCE_CREATE_ENUMERATE_PORTABILITY_BIT_KHR;
#endif

        printf("Requested instance extensions:\n");
        for (uint32_t i = 0; i < num_exts; ++i) {
            printf("    %s\n", exts[i]);
        }

        // The VK_LAYER_KHRONOS_validation validation layer helps detect incorrect API usage. It's
        // extremely helpful in development, but not supported on every system. Enable it if it's
        // available.

        const char *validation_layer     = "VK_LAYER_KHRONOS_validation";
        bool        has_validation_layer = false;
        {
            uint32_t num_layers = 0;
            vkEnumerateInstanceLayerProperties(&num_layers, 0);

            VkLayerProperties *layers = calloc(num_layers, sizeof(VkLayerProperties));
            vkEnumerateInstanceLayerProperties(&num_layers, layers);

            for (uint32_t i = 0; i < num_layers; ++i) {
                if (!strcmp(layers[i].layerName, validation_layer)) {
                    has_validation_layer = true;
                    break;
                }
            }

            free(layers);
        }

        // This function just passes info the vkCreateInstance. Specify required instance
        // extensions and validation layers here.
        VkInstanceCreateInfo create_info = {
            .sType                   = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO,
            .flags                   = flags,
            .pApplicationInfo        = &app_info,
            .enabledExtensionCount   = num_exts,
            .ppEnabledExtensionNames = exts,
            .enabledLayerCount       = has_validation_layer ? 1 : 0,
            .ppEnabledLayerNames     = &validation_layer,
        };
        VkResult vkr = vkCreateInstance(&create_info, 0, &vki);
        if (vkr != VK_SUCCESS) {
            printf("vkCreateInstance failed: %d", vkr);
            return 0;
        }

        #undef REQUIRE_EXTENSION
    }

    // Create the window
    const uint32_t wndflags = SDL_WINDOW_VULKAN | SDL_WINDOW_RESIZABLE;
    SDL_Window *wnd = SDL_CreateWindow("vk-cube", 1024, 768, wndflags);
    if (!wnd) {
        printf("Failed to create window: %s\n", SDL_GetError());
        return 0;
    }

    // Create the surface now so we can check if the physical device and queue families support
    // drawing to it.
    VkSurfaceKHR vksurf = 0;
    if (!SDL_Vulkan_CreateSurface(wnd, vki, 0, &vksurf)) {
        printf("Failed to create Vulkan surface: %s\n", SDL_GetError());
        return 0;
    }

    // Image formats
    VkFormat swapchain_format = VK_FORMAT_B8G8R8A8_SRGB;
    VkFormat depth_format     = VK_FORMAT_D32_SFLOAT;

    // Select physical device and queue family
    //
    // The physical device is the literal GPU hardware unit that support Vulkan. I'm just selecting
    // the first one with dynamic rendering support. In a real app, you might want to make it more
    // complex and try to select the best GPU. Or better yet, allow the user to select the GPU and
    // match the device UUID in VkPhysicalDeviceProperties.
    //
    // Queue families essentially just describe what operations a given device supports. This is
    // important for nuanced things like compute or video, but this isn't really critical when we
    // just want to draw basic 3D graphics. Like the device, just support the first queue family
    // with VK_QUEUE_GRAPHICS_BIT support.
    VkPhysicalDevice vkpdev = 0;
    uint32_t         vkqfi  = UINT32_MAX;
    {
        // Enumerate physical devices
        uint32_t num_devs = 0;
        vkEnumeratePhysicalDevices(vki, &num_devs, 0);

        VkPhysicalDevice *devs = calloc(num_devs, sizeof(VkPhysicalDevice));
        vkEnumeratePhysicalDevices(vki, &num_devs, devs);

        printf("Available GPUs:\n");
        for (uint32_t i = 0; i < num_devs; ++i) {
            // Get basic device properties (name)
            VkPhysicalDeviceProperties2 properties = {
                .sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_PROPERTIES_2,
            };
            vkGetPhysicalDeviceProperties2(devs[i], &properties);

            // Get dynamic rendering support
            VkPhysicalDeviceDynamicRenderingFeatures dynamic_rendering_features = {
                .sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_DYNAMIC_RENDERING_FEATURES,
            };
            // and Synchronization2 support
            VkPhysicalDeviceSynchronization2Features sync2_features = {
                .sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_SYNCHRONIZATION_2_FEATURES,
                .pNext = &dynamic_rendering_features,
            };
            VkPhysicalDeviceFeatures2 features = {
                .sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_FEATURES_2,
                .pNext = &sync2_features,
            };
            vkGetPhysicalDeviceFeatures2(devs[i], &features);

            // Get device queue families
            uint32_t num_qfams = 0;
            vkGetPhysicalDeviceQueueFamilyProperties(devs[i], &num_qfams, 0);

            VkQueueFamilyProperties *qfams = calloc(num_qfams, sizeof(VkQueueFamilyProperties));
            vkGetPhysicalDeviceQueueFamilyProperties(devs[i], &num_qfams, qfams);

            uint32_t dev_qfi = UINT32_MAX;
            for (uint32_t j = 0; j < num_qfams; ++j) {
                if (!(qfams[j].queueFlags & VK_QUEUE_GRAPHICS_BIT)) {
                    continue;
                }

                if (SDL_Vulkan_GetPresentationSupport(vki, devs[i], j)) {
                    dev_qfi = j;
                }
            }

            free(qfams);

            bool selected = !vkpdev && dev_qfi != UINT32_MAX &&
                            dynamic_rendering_features.dynamicRendering &&
                            sync2_features.synchronization2;

            printf("    %s%s\n", properties.properties.deviceName, selected ? " (selected)" : "");

            if (selected) {
                vkpdev = devs[i];
                vkqfi  = dev_qfi;
            }
        }
        free(devs);
    }

    // At this point our validation layers are loaded and I'm not going to check VkResult

    // Create the device instance
    VkDevice vkdev = 0;
    {
        const char *exts[] = {
            "VK_KHR_swapchain",          // required to present stuff to the screen
#ifdef __APPLE__
            "VK_KHR_portability_subset", // required for MoltenVK
#endif
        };
        printf("Requested device extensions:\n");
        for (uint32_t i = 0; i < COUNTOF(exts); ++i) {
            printf("    %s\n", exts[i]);
        }

        // Ask for dynamic rendering support
        VkPhysicalDeviceDynamicRenderingFeatures dynamic_rendering_features = {
            .sType            = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_DYNAMIC_RENDERING_FEATURES,
            .dynamicRendering = VK_TRUE,
        };
        // Ask for Synchronization2 support
        VkPhysicalDeviceSynchronization2Features sync2_features = {
            .sType            = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_SYNCHRONIZATION_2_FEATURES,
            .synchronization2 = VK_TRUE,
            .pNext            = &dynamic_rendering_features,
        };

        float queue_priority = 1.0f;
        VkDeviceQueueCreateInfo queue_create_info = {
            .sType            = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO,
            .queueFamilyIndex = vkqfi,
            .queueCount       = 1,
            .pQueuePriorities = &queue_priority,
        };
        VkDeviceCreateInfo create_info = {
            .sType                   = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO,
            .queueCreateInfoCount    = 1,
            .pQueueCreateInfos       = &queue_create_info,
            .pNext                   = &sync2_features,
            .enabledExtensionCount   = COUNTOF(exts),
            .ppEnabledExtensionNames = exts,
        };
        vkCreateDevice(vkpdev, &create_info, 0, &vkdev);

        printf("Logical device created\n");
    }

    // Get handle to graphics queue for the logical device
    VkQueue vkq = 0;
    vkGetDeviceQueue(vkdev, vkqfi, 0, &vkq);

    // Allow two frames in flight. This means we can start preparing the next CPU-side while
    // waiting for the GPU to render the last frame;
    const uint32_t max_frames_in_flight = 2;

    // Create command pool and buffers.
    //
    // The command pool is simply a memory allocator for GPU commands.
    //
    // The command buffer is the actual list of commands that will later be queued for execution on
    // the GPU. With max_frames_in_flight = 2, we will need 2 command buffers since we will be
    // rendering two frames at the same time.
    VkCommandPool    vkcmdpool = 0;
    VkCommandBuffer *vkcmdbufs = calloc(max_frames_in_flight, sizeof(VkCommandBuffer));
    {
        VkCommandPoolCreateInfo create_pool = {
            .sType            = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO,
            .flags            = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT,
            .queueFamilyIndex = vkqfi,
        };
        vkCreateCommandPool(vkdev, &create_pool, 0, &vkcmdpool);

        VkCommandBufferAllocateInfo allocate_buffer = {
            .sType              = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO,
            .commandPool        = vkcmdpool,
            .level              = VK_COMMAND_BUFFER_LEVEL_PRIMARY,
            .commandBufferCount = max_frames_in_flight,
        };
        vkAllocateCommandBuffers(vkdev, &allocate_buffer, vkcmdbufs);

        printf("Command buffers created\n");
    }

    typedef struct Vertex Vertex;
    struct Vertex { float p[3]; float c[3]; float n[3]; };

    // Model data for a unit cube
    const Vertex vdata[] = {
        // front
        { { -0.5f, -0.5f,  0.5f }, { 1.0f, 0.0f, 0.0f }, {  0.0f,  0.0f,  1.0f } },
        { {  0.5f, -0.5f,  0.5f }, { 1.0f, 0.0f, 0.0f }, {  0.0f,  0.0f,  1.0f } },
        { {  0.5f,  0.5f,  0.5f }, { 1.0f, 0.0f, 0.0f }, {  0.0f,  0.0f,  1.0f } },
        { { -0.5f,  0.5f,  0.5f }, { 1.0f, 0.0f, 0.0f }, {  0.0f,  0.0f,  1.0f } },
        // back
        { {  0.5f, -0.5f, -0.5f }, { 0.0f, 1.0f, 0.0f }, {  0.0f,  0.0f, -1.0f } },
        { { -0.5f, -0.5f, -0.5f }, { 0.0f, 1.0f, 0.0f }, {  0.0f,  0.0f, -1.0f } },
        { { -0.5f,  0.5f, -0.5f }, { 0.0f, 1.0f, 0.0f }, {  0.0f,  0.0f, -1.0f } },
        { {  0.5f,  0.5f, -0.5f }, { 0.0f, 1.0f, 0.0f }, {  0.0f,  0.0f, -1.0f } },
        // left (blue)
        { { -0.5f, -0.5f, -0.5f }, { 0.0f, 0.0f, 1.0f }, { -1.0f,  0.0f,  0.0f } },
        { { -0.5f, -0.5f,  0.5f }, { 0.0f, 0.0f, 1.0f }, { -1.0f,  0.0f,  0.0f } },
        { { -0.5f,  0.5f,  0.5f }, { 0.0f, 0.0f, 1.0f }, { -1.0f,  0.0f,  0.0f } },
        { { -0.5f,  0.5f, -0.5f }, { 0.0f, 0.0f, 1.0f }, { -1.0f,  0.0f,  0.0f } },
        // right (yellow)
        { {  0.5f, -0.5f,  0.5f }, { 1.0f, 1.0f, 0.0f }, {  1.0f,  0.0f,  0.0f } },
        { {  0.5f, -0.5f, -0.5f }, { 1.0f, 1.0f, 0.0f }, {  1.0f,  0.0f,  0.0f } },
        { {  0.5f,  0.5f, -0.5f }, { 1.0f, 1.0f, 0.0f }, {  1.0f,  0.0f,  0.0f } },
        { {  0.5f,  0.5f,  0.5f }, { 1.0f, 1.0f, 0.0f }, {  1.0f,  0.0f,  0.0f } },
        // top (magenta)
        { { -0.5f,  0.5f,  0.5f }, { 1.0f, 0.0f, 1.0f }, {  0.0f,  1.0f,  0.0f } },
        { {  0.5f,  0.5f,  0.5f }, { 1.0f, 0.0f, 1.0f }, {  0.0f,  1.0f,  0.0f } },
        { {  0.5f,  0.5f, -0.5f }, { 1.0f, 0.0f, 1.0f }, {  0.0f,  1.0f,  0.0f } },
        { { -0.5f,  0.5f, -0.5f }, { 1.0f, 0.0f, 1.0f }, {  0.0f,  1.0f,  0.0f } },
        // bottom (cyan)
        { { -0.5f, -0.5f, -0.5f }, { 0.0f, 1.0f, 1.0f }, {  0.0f, -1.0f,  0.0f } },
        { {  0.5f, -0.5f, -0.5f }, { 0.0f, 1.0f, 1.0f }, {  0.0f, -1.0f,  0.0f } },
        { {  0.5f, -0.5f,  0.5f }, { 0.0f, 1.0f, 1.0f }, {  0.0f, -1.0f,  0.0f } },
        { { -0.5f, -0.5f,  0.5f }, { 0.0f, 1.0f, 1.0f }, {  0.0f, -1.0f,  0.0f } }
    };

    const uint16_t idata[] = {
         0,  1,  2,   0,  2,  3, // front
         4,  5,  6,   4,  6,  7, // back
         8,  9, 10,   8, 10, 11, // left
        12, 13, 14,  12, 14, 15, // right
        16, 17, 18,  16, 18, 19, // top
        20, 21, 22,  20, 22, 23, // bottom
    };

    // Uniform data
    typedef struct Uniforms Uniforms;
    struct Uniforms
    {
        float mvp[16];
        float model[16];
    };

    // Alllocate memory for vertex, index, and uniform data
    //
    // Note: vkAllocateMemory is very expensive, and there's a hard limit to how many times it can
    // be called. In a real app, it's better to do bulk allocations and sub-allocate as needed.
    // Theres'a a library called "vulkan memory allocator" that people really like. For this demo,
    // allocating per buffer is fine.
    
    VkBuffer       vkvbuf = 0; // cube vertex buffer
    VkDeviceMemory vkvmem = 0;
    VkBuffer       vkibuf = 0; // cube index buffer
    VkDeviceMemory vkimem = 0;

    VkBuffer       *vkubufs = calloc(max_frames_in_flight, sizeof(VkBuffer));
    VkDeviceMemory *vkumems = calloc(max_frames_in_flight, sizeof(VkDeviceMemory));

    {
        VkPhysicalDeviceMemoryProperties memprops = { 0 };
        vkGetPhysicalDeviceMemoryProperties(vkpdev, &memprops);

        // This code is super long for what it does, so make it data-driven. It would be cleaner
        // as a function, but I want this demo to read sequentually.

        typedef struct Alloc Alloc;
        struct Alloc
        {
            VkBuffer           *buf;
            VkDeviceMemory     *mem;
            VkDeviceSize       size;
            VkBufferUsageFlags usage;
        };

        uint32_t num_allocs = 0;
        Alloc    allocs[32] = { 0 };

        #define ALLOC(BUF, MEM, SIZE, USAGE)                         \
            ASSERT(num_allocs< COUNTOF(allocs));                     \
            allocs[num_allocs++] = (Alloc){ BUF, MEM, SIZE, USAGE };

        ALLOC(&vkvbuf, &vkvmem, sizeof(vdata), VK_BUFFER_USAGE_VERTEX_BUFFER_BIT);
        ALLOC(&vkibuf, &vkimem, sizeof(idata), VK_BUFFER_USAGE_INDEX_BUFFER_BIT);
        for (uint32_t i = 0; i < max_frames_in_flight; ++i) {
            ALLOC(&vkubufs[i], &vkumems[i], sizeof(Uniforms), VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT);
        }

        for (uint32_t i = 0; i < num_allocs; ++i) {
            VkBufferCreateInfo create = {
                .sType       = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO,
                .size        = allocs[i].size,
                .usage       = allocs[i].usage,
                .sharingMode = VK_SHARING_MODE_EXCLUSIVE,
            };
            vkCreateBuffer(vkdev, &create, 0, allocs[i].buf);

            // Actual allocation size including padding and alignment
            VkMemoryRequirements memreq = { 0 };
            vkGetBufferMemoryRequirements(vkdev, *allocs[i].buf, &memreq);

            // Find the appropriate device memory type for this allocation
            VkMemoryPropertyFlagBits required_props = VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT |
                                                      VK_MEMORY_PROPERTY_HOST_COHERENT_BIT;
            uint32_t mem_type_idx = find_mem_type(vkpdev, memreq.memoryTypeBits, required_props);

            VkMemoryAllocateInfo alloc = {
                .sType           = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO,
                .allocationSize  = memreq.size,
                .memoryTypeIndex = mem_type_idx,
            };
            vkAllocateMemory(vkdev, &alloc, 0, allocs[i].mem);
            vkBindBufferMemory(vkdev, *allocs[i].buf, *allocs[i].mem, 0);
        }

        #undef ALLOC

        printf("Geometry buffers created\n");
    }

    // Upload vertex data
    {
        void *map = 0;
        vkMapMemory(vkdev, vkvmem, 0, sizeof(vdata), 0, &map);
        memcpy(map, vdata, sizeof(vdata));
        vkUnmapMemory(vkdev, vkvmem);
    }
    
    // Upload index data
    {
        void *map = 0;
        vkMapMemory(vkdev, vkimem, 0, sizeof(idata), 0, &map);
        memcpy(map, idata, sizeof(idata));
        vkUnmapMemory(vkdev, vkimem);
    }

    // Map uniform buffers
    Uniforms **ubufs = calloc(max_frames_in_flight, sizeof(Uniforms *));
    for (uint32_t i = 0; i < max_frames_in_flight; ++i) {
        vkMapMemory(vkdev, vkumems[i], 0, sizeof(Uniforms), 0, (void **)&ubufs[i]);
    }

    // Create descriptors
    //
    // Descriptors specify how a shader can access a resource. In this case, it only needs to
    // know how to read uniforms in the vertex stage.
    //
    // VkDescriptorSetLayout defines how the binding is used
    // VkDescriptorPool is an allocator for descriptor sets
    // VkDescriptorSet defines the pointer to the actual block of GPU device memory is used
    VkDescriptorSetLayout vksetlayout = 0;
    VkDescriptorPool      vkdescpool  = 0;
    VkDescriptorSet      *vksets      = calloc(max_frames_in_flight, sizeof(VkDescriptorSet));
    {
        VkDescriptorSetLayoutBinding descriptor_set_layout_binding = {
            .binding         = 0,
            .descriptorType  = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
            .descriptorCount = 1,
            .stageFlags      = VK_SHADER_STAGE_VERTEX_BIT
        };
        VkDescriptorSetLayoutCreateInfo descriptor_set_layout_create = {
            .sType        = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO,
            .bindingCount = 1,
            .pBindings    = &descriptor_set_layout_binding
        };
        vkCreateDescriptorSetLayout(vkdev, &descriptor_set_layout_create, 0, &vksetlayout);

        // Allocator for descriptor sets
        VkDescriptorPoolSize pool_size = {
            .type            = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
            .descriptorCount = max_frames_in_flight
        };
        VkDescriptorPoolCreateInfo pool_create = {
            .sType         = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO,
            .maxSets       = max_frames_in_flight,
            .poolSizeCount = 1,
            .pPoolSizes    = &pool_size
        };
        vkCreateDescriptorPool(vkdev, &pool_create, 0, &vkdescpool);

        VkDescriptorSetLayout *layouts = calloc(max_frames_in_flight,
                                                sizeof(VkDescriptorSetLayout));
        for (uint32_t i = 0; i < max_frames_in_flight; ++i) {
            layouts[i] = vksetlayout;
        }

        VkDescriptorSetAllocateInfo set_alloc_info = {
            .sType              = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO,
            .descriptorPool     = vkdescpool,
            .descriptorSetCount = max_frames_in_flight,
            .pSetLayouts        = layouts
        };
        vkAllocateDescriptorSets(vkdev, &set_alloc_info, vksets);

        // Point each descriptor set to its respective uniform buffer
        for (uint32_t i = 0; i < max_frames_in_flight; ++i) {
            VkDescriptorBufferInfo buffer_info = {
                .buffer = vkubufs[i],
                .offset = 0,
                .range  = sizeof(Uniforms)
            };
            VkWriteDescriptorSet write = {
                .sType           = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
                .dstSet          = vksets[i],
                .dstBinding      = 0,
                .descriptorType  = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
                .descriptorCount = 1,
                .pBufferInfo     = &buffer_info
            };
            vkUpdateDescriptorSets(vkdev, 1, &write, 0, 0);
        }
        printf("Descriptor sets created\n");
    }

    // Create pipeline
    VkPipelineLayout vklayout = 0;
    VkPipeline       vkpl     = 0;
    {
        // Vertex shader module
        VkShaderModule vs_mod = 0;
        VkShaderModuleCreateInfo vs_create = {
            .sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO
        };
        vs_create.pCode = SDL_LoadFile("vk-cube-vs.spv", &vs_create.codeSize);
        if (!vs_create.pCode) {
            printf("Failed to load vertex shader: %s\n", SDL_GetError());
            return 0;
        }
        vkCreateShaderModule(vkdev, &vs_create, 0, &vs_mod);

        VkPipelineShaderStageCreateInfo vs_stage = {
            .sType  = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO,
            .stage  = VK_SHADER_STAGE_VERTEX_BIT,
            .module = vs_mod,
            .pName  = "main"
        };

        // Fragment shader module
        VkShaderModule fs_mod = 0;
        VkShaderModuleCreateInfo fs_create = {
            .sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO,
        };
        fs_create.pCode = SDL_LoadFile("vk-cube-fs.spv", &fs_create.codeSize);
        if (!fs_create.pCode) {
            printf("Failed to load fragment shader: %s\n", SDL_GetError());
            return 0;
        }
        vkCreateShaderModule(vkdev, &fs_create, 0, &fs_mod);

        VkPipelineShaderStageCreateInfo fs_stage = {
            .sType  = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO,
            .stage  = VK_SHADER_STAGE_FRAGMENT_BIT,
            .module = fs_mod,
            .pName  = "main"
        };

        VkPipelineShaderStageCreateInfo stages[] = { vs_stage, fs_stage };

        // Define vertex input
        VkVertexInputBindingDescription vert_bind_desc = {
            .binding   = 0,
            .stride    = sizeof(Vertex),
            .inputRate = VK_VERTEX_INPUT_RATE_VERTEX
        };

        // Vertex attribute: position
        VkVertexInputAttributeDescription vert_attr_p = {
            .binding  = 0,
            .location = 0,
            .format   = VK_FORMAT_R32G32B32_SFLOAT,
            .offset   = offsetof(Vertex, p)
        };
        // Vertex attribute: color
        VkVertexInputAttributeDescription vert_attr_c = {
            .binding  = 0,
            .location = 1,
            .format   = VK_FORMAT_R32G32B32_SFLOAT,
            .offset   = offsetof(Vertex, c)
        };
        // Vertex attribute: normal
        VkVertexInputAttributeDescription vert_attr_n = {
            .binding  = 0,
            .location = 2,
            .format   = VK_FORMAT_R32G32B32_SFLOAT,
            .offset   = offsetof(Vertex, n)
        };

        VkVertexInputAttributeDescription vert_attrs[] = {
            vert_attr_p,
            vert_attr_c,
            vert_attr_n
        };

        VkPipelineVertexInputStateCreateInfo vert_create = {
            .sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO,
            .vertexBindingDescriptionCount   = 1,
            .pVertexBindingDescriptions      = &vert_bind_desc,
            .vertexAttributeDescriptionCount = COUNTOF(vert_attrs),
            .pVertexAttributeDescriptions    = vert_attrs
        };

        // Input geometry layout
        VkPipelineInputAssemblyStateCreateInfo input_assembly_create = {
            .sType    = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO,
            .topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST
        };

        // Dynamic viewport and scissor state
        VkDynamicState dynamic_states[] = {
            VK_DYNAMIC_STATE_VIEWPORT,
            VK_DYNAMIC_STATE_SCISSOR
        };
        VkPipelineDynamicStateCreateInfo dynamic_state_create = {
            .sType             = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO,
            .dynamicStateCount = COUNTOF(dynamic_states),
            .pDynamicStates    = dynamic_states
        };
        VkPipelineViewportStateCreateInfo viewport_state_create = {
            .sType         = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO,
            .viewportCount = 1,
            .scissorCount  = 1
        };

        // Rasterizer state
        VkPipelineRasterizationStateCreateInfo rasterizer_state_create = {
            .sType       = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO,
            .polygonMode = VK_POLYGON_MODE_FILL,
            .cullMode    = VK_CULL_MODE_BACK_BIT,
            .frontFace   = VK_FRONT_FACE_COUNTER_CLOCKWISE,
            .lineWidth   = 1.0f
        };

        // Multisample state
        VkPipelineMultisampleStateCreateInfo multisample_state_create = {
            .sType                = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO,
            .rasterizationSamples = VK_SAMPLE_COUNT_1_BIT // disabled
        };

        // Depth stencil state
        VkPipelineDepthStencilStateCreateInfo depth_stencil_state_create = {
            .sType            = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO,
            .depthTestEnable  = VK_TRUE,
            .depthWriteEnable = VK_TRUE,
            .depthCompareOp   = VK_COMPARE_OP_LESS
        };

        // Color blending state
        VkPipelineColorBlendAttachmentState color_blend_attachment_state = {
            .colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT |
                              VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT,
        };
        VkPipelineColorBlendStateCreateInfo color_blend_state_create = {
            .sType           = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO,
            .attachmentCount = 1,
            .pAttachments    = &color_blend_attachment_state
        };

        // Pipeline layout - basically just specifies descriptor set layout
        VkPipelineLayoutCreateInfo layout_create = {
            .sType          = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO,
            .setLayoutCount = 1,
            .pSetLayouts    = &vksetlayout
        };
        vkCreatePipelineLayout(vkdev, &layout_create, 0, &vklayout);

        // Rendering state
        VkPipelineRenderingCreateInfo rendering_create = {
            .sType                   = VK_STRUCTURE_TYPE_PIPELINE_RENDERING_CREATE_INFO,
            .colorAttachmentCount    = 1,
            .pColorAttachmentFormats = &swapchain_format,
            .depthAttachmentFormat   = depth_format
        };

        // Assemble everything
        VkGraphicsPipelineCreateInfo pipeline_create = {
            .sType               = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO,
            .pNext               = &rendering_create,
            .stageCount          = COUNTOF(stages),
            .pStages             = stages,
            .pVertexInputState   = &vert_create,
            .pInputAssemblyState = &input_assembly_create,
            .pViewportState      = &viewport_state_create,
            .pRasterizationState = &rasterizer_state_create,
            .pMultisampleState   = &multisample_state_create,
            .pDepthStencilState  = &depth_stencil_state_create,
            .pColorBlendState    = &color_blend_state_create,
            .pDynamicState       = &dynamic_state_create,
            .layout              = vklayout
        };
        vkCreateGraphicsPipelines(vkdev, 0, 1, &pipeline_create, 0, &vkpl);

        printf("Pipeline created\n");
    }

    // The swapchain needs to be recreated any time the window is resized
    bool           swapchain_dirty      = true;
    VkSwapchainKHR vkswapchain          = 0;
    uint32_t       num_swapchain_images = 0;
    VkImage       *swapchain_images     = 0;
    VkImageView   *swapchain_views      = 0;
    VkImage        depth_image          = 0;
    VkDeviceMemory depth_mem            = 0;
    VkImageView    depth_view           = 0;
    VkExtent2D     extent2              = { 0 };
    VkExtent3D     extent3              = { 0 };

    // Signaled when the swapchain has fresh image to render to
    VkSemaphore *vk_image_available_sems = calloc(max_frames_in_flight, sizeof(VkSemaphore));

    // Signaled when we are done drawing to an image and it should be presented to the user
    VkSemaphore *vk_render_finished_sems = 0;
    uint32_t     num_render_finished_sems = 0;

    // Signaled when the command buffer is done executing. Signaled by default to avoid deadlock
    // on first frame.
    VkFence *vk_in_flight_fences = calloc(max_frames_in_flight, sizeof(VkFence));

    // Initial allocations for both
    for (uint32_t i = 0; i < max_frames_in_flight; ++i) {
        VkFenceCreateInfo fci = {
            .sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO,
            .flags = VK_FENCE_CREATE_SIGNALED_BIT,
        };
        vkCreateFence(vkdev, &fci, 0, &vk_in_flight_fences[i]);

        VkSemaphoreCreateInfo sci = {
            .sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO,
        };
        vkCreateSemaphore(vkdev, &sci, 0, &vk_image_available_sems[i]);
    }

    bool running = true;
    while (running) {
        SDL_Event evt;
        while (SDL_PollEvent(&evt)) {
            switch (evt.type) {
                case SDL_EVENT_WINDOW_RESIZED:
                    swapchain_dirty = true;
                    break;
                case SDL_EVENT_QUIT:
                    running = false;
                    break;
            };
        }

        int wnd_w = 0;
        int wnd_h = 0;
        SDL_GetWindowSizeInPixels(wnd, &wnd_w, &wnd_h);
        
        if (wnd_w <= 0 || wnd_h <= 0) {
            SDL_Delay(10); // 10ms, idk
            continue;
        }

        // Create swapchain if needed
        if (swapchain_dirty) {
            vkDeviceWaitIdle(vkdev);

            VkSurfaceCapabilitiesKHR scaps;
            vkGetPhysicalDeviceSurfaceCapabilitiesKHR(vkpdev, vksurf, &scaps);

            assert(scaps.currentExtent.width  > 0);
            assert(scaps.currentExtent.height > 0);

            if (vkswapchain) {
                vkDestroyImageView(vkdev, depth_view, 0); depth_view = 0;
                vkDestroyImage(vkdev, depth_image, 0); depth_image = 0;
                vkFreeMemory(vkdev, depth_mem, 0); depth_mem = 0;
                for (uint32_t i = 0; i < num_swapchain_images; ++i) {
                    vkDestroyImageView(vkdev, swapchain_views[i], 0);
                    swapchain_views[i] = 0;
                }
                free(swapchain_images); swapchain_images = 0;
                free(swapchain_views);  swapchain_views  = 0;
                vkDestroySwapchainKHR(vkdev, vkswapchain, 0); vkswapchain = 0;
            }

            // minImageCount is almost always 2
            uint32_t image_count = scaps.minImageCount + 1;
            if (scaps.maxImageCount > 0) {
                image_count = MIN(image_count, scaps.maxImageCount);
            }
            assert(max_frames_in_flight <= image_count);

            extent2.width  = wnd_w;
            extent2.height = wnd_h;
            extent3.width  = extent2.width;
            extent3.height = extent2.height;
            extent3.depth  = 1;

            VkSwapchainCreateInfoKHR swapchain_create = {
                .sType            = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR,
                .surface          = vksurf,
                .minImageCount    = image_count,
                .imageFormat      = swapchain_format,
                .imageColorSpace  = VK_COLOR_SPACE_SRGB_NONLINEAR_KHR,
                .imageExtent      = extent2,
                .imageArrayLayers = 1,
                .imageUsage       = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT,
                .imageSharingMode = VK_SHARING_MODE_EXCLUSIVE, // gfx and present queues are same
                .preTransform     = scaps.currentTransform,
                .compositeAlpha   = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR,
                .presentMode      = VK_PRESENT_MODE_FIFO_KHR, // vsync
                .clipped          = VK_TRUE
            };
            vkCreateSwapchainKHR(vkdev, &swapchain_create, 0, &vkswapchain);

            // Get swapchain image handles
            vkGetSwapchainImagesKHR(vkdev, vkswapchain, &num_swapchain_images, 0);
            swapchain_images = calloc(num_swapchain_images, sizeof(VkImage));
            vkGetSwapchainImagesKHR(vkdev, vkswapchain, &num_swapchain_images, swapchain_images);

            // Create swapchain image views
            swapchain_views = calloc(num_swapchain_images, sizeof(VkImageView));
            for (uint32_t i = 0; i < num_swapchain_images; ++i) {
                VkImageViewCreateInfo view_create = {
                    .sType            = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO,
                    .image            = swapchain_images[i],
                    .viewType         = VK_IMAGE_VIEW_TYPE_2D,
                    .format           = swapchain_format,
                    .subresourceRange = (VkImageSubresourceRange){
                        .aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
                        .levelCount = 1,
                        .layerCount = 1
                    }
                };
                vkCreateImageView(vkdev, &view_create, 0, &swapchain_views[i]);
            }

            // Create depth image
            VkImageCreateInfo depth_create = {
                .sType       = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO,
                .imageType   = VK_IMAGE_TYPE_2D,
                .format      = depth_format,
                .extent      = extent3,
                .mipLevels   = 1,
                .arrayLayers = 1,
                .samples     = VK_SAMPLE_COUNT_1_BIT,
                .tiling      = VK_IMAGE_TILING_OPTIMAL,
                .usage       = VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT
            };
            vkCreateImage(vkdev, &depth_create, 0, &depth_image);

            // Allocate depth image memory
            VkMemoryRequirements memreq = { 0 };
            vkGetImageMemoryRequirements(vkdev, depth_image, &memreq);
            VkMemoryAllocateInfo alloc = {
                .sType          = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO,
                .allocationSize = memreq.size
            };
            alloc.memoryTypeIndex = find_mem_type(vkpdev, memreq.memoryTypeBits,
                                                  VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);
            vkAllocateMemory(vkdev, &alloc, 0, &depth_mem);
            vkBindImageMemory(vkdev, depth_image, depth_mem, 0);

            // Create depth image view
            VkImageViewCreateInfo view_create = {
                .sType            = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO,
                .image            = depth_image,
                .viewType         = VK_IMAGE_VIEW_TYPE_2D,
                .format           = depth_format,
                .subresourceRange = (VkImageSubresourceRange){
                    .aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT,
                    .levelCount = 1,
                    .layerCount = 1
                }
            };
            vkCreateImageView(vkdev, &view_create, 0, &depth_view);

            // Create synchronization objects
            //
            // Semaphores are for GPU-GPU synchronization and fences are for CPU-GPU sync.
            {
                // The spec allows num_swapchain_images to vary per frame, but it probably won't.
                // Deal with it anyway.
                if (num_render_finished_sems < num_swapchain_images) {
                    vk_render_finished_sems = realloc(vk_render_finished_sems,
                                                      sizeof(VkSemaphore) * num_swapchain_images);
                    for (uint32_t i = num_render_finished_sems; i < num_swapchain_images; ++i) {
                        VkSemaphoreCreateInfo sci = {
                            .sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO,
                        };
                        vkCreateSemaphore(vkdev, &sci, 0, &vk_render_finished_sems[i]);
                    }

                    num_render_finished_sems = num_swapchain_images;
                }
            }

            printf("Swapchain created\n");

            swapchain_dirty = false;
        }

        static int f = 0; // frame cycler, [0, max_frames_in_flight)

        vkWaitForFences(vkdev, 1, &vk_in_flight_fences[f], VK_TRUE, UINT64_MAX);

        uint32_t img_idx = 0;
        VkResult vkr = vkAcquireNextImageKHR(vkdev, vkswapchain, UINT64_MAX,
                                             vk_image_available_sems[f], VK_NULL_HANDLE, &img_idx);
        if (vkr == VK_ERROR_OUT_OF_DATE_KHR) {
            swapchain_dirty = true;
            continue;
        }

        vkResetFences(vkdev, 1, &vk_in_flight_fences[f]);

        // Update MVP
        float *mvp   = ubufs[f]->mvp;
        float *model = ubufs[f]->model;
        {
            const float t = (float)SDL_GetTicks();

            float xyz[3] = { SDL_cosf(t * 0.001f), SDL_sinf(t * 0.001f), -2.0f };
            float translate[16];
            mat4translate(translate, xyz);

            float rotate_x[16];
            mat4rotx(rotate_x, DEG2RAD(t * 0.08f));

            float rotate_y[16];
            mat4roty(rotate_y, DEG2RAD(t * 0.05f));

            float tmp[16];
            mat4mul(tmp, rotate_x, rotate_y);
            mat4mul(model, translate, tmp);

            float proj[16];
            mat4perspective(proj, DEG2RAD(90.0f), (float)wnd_w / (float)wnd_h, 0.1f, 10.0f);

            mat4mul(mvp, proj, model);
        }

        VkCommandBuffer cmd = vkcmdbufs[f];
        vkResetCommandBuffer(cmd, 0);

        VkCommandBufferBeginInfo cmd_begin = {
            .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO
        };
        vkBeginCommandBuffer(cmd, &cmd_begin);

        // Transition swapchain image: unknown -> color attachment
        {
            VkImageMemoryBarrier2 barrier = {
                .sType               = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER_2,
                .srcStageMask        = VK_PIPELINE_STAGE_2_TOP_OF_PIPE_BIT,
                .srcAccessMask       = 0,
                .dstStageMask        = VK_PIPELINE_STAGE_2_COLOR_ATTACHMENT_OUTPUT_BIT,
                .dstAccessMask       = VK_ACCESS_2_COLOR_ATTACHMENT_WRITE_BIT,
                .oldLayout           = VK_IMAGE_LAYOUT_UNDEFINED,
                .newLayout           = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
                .srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
                .dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
                .image               = swapchain_images[img_idx],
                .subresourceRange    = (VkImageSubresourceRange){
                    .aspectMask     = VK_IMAGE_ASPECT_COLOR_BIT,
                    .baseMipLevel   = 0,
                    .levelCount     = 1,
                    .baseArrayLayer = 0,
                    .layerCount     = 1
                }
            };

            VkDependencyInfo dep_info = {
                .sType                   = VK_STRUCTURE_TYPE_DEPENDENCY_INFO,
                .imageMemoryBarrierCount = 1,
                .pImageMemoryBarriers    = &barrier
            };
            vkCmdPipelineBarrier2(cmd, &dep_info);
        }

        // Transition depth image: unknown -> depth attachment
        {
            VkImageMemoryBarrier2 barrier = {
                .sType               = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER_2,
                .srcStageMask        = VK_PIPELINE_STAGE_2_TOP_OF_PIPE_BIT,
                .srcAccessMask       = 0,
                .dstStageMask        = VK_PIPELINE_STAGE_2_EARLY_FRAGMENT_TESTS_BIT,
                .dstAccessMask       = VK_ACCESS_2_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT,
                .oldLayout           = VK_IMAGE_LAYOUT_UNDEFINED,
                .newLayout           = VK_IMAGE_LAYOUT_DEPTH_ATTACHMENT_OPTIMAL,
                .srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
                .dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
                .image               = depth_image,
                .subresourceRange    = (VkImageSubresourceRange){
                    .aspectMask     = VK_IMAGE_ASPECT_DEPTH_BIT,
                    .baseMipLevel   = 0,
                    .levelCount     = 1,
                    .baseArrayLayer = 0,
                    .layerCount     = 1
                }
            };

            VkDependencyInfo dep_info = {
                .sType                   = VK_STRUCTURE_TYPE_DEPENDENCY_INFO,
                .imageMemoryBarrierCount = 1,
                .pImageMemoryBarriers    = &barrier
            };
            vkCmdPipelineBarrier2(cmd, &dep_info);
        }

        // Begin dynamic rendering
        {
            VkRenderingAttachmentInfo color_attachment = {
                .sType            = VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO,
                .imageView        = swapchain_views[img_idx],
                .imageLayout      = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
                .loadOp           = VK_ATTACHMENT_LOAD_OP_CLEAR,
                .storeOp          = VK_ATTACHMENT_STORE_OP_STORE,
                .clearValue.color = { { 0.1f, 0.1f, 0.1f, 1.0f } }
            };

            VkRenderingAttachmentInfo depth_attachment = {
                .sType                   = VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO,
                .imageView               = depth_view,
                .imageLayout             = VK_IMAGE_LAYOUT_DEPTH_ATTACHMENT_OPTIMAL,
                .loadOp                  = VK_ATTACHMENT_LOAD_OP_CLEAR,
                .storeOp                 = VK_ATTACHMENT_STORE_OP_DONT_CARE,
                .clearValue.depthStencil = { 1.0f, 0 }
            };

            VkRenderingInfo render_info = {
                .sType      = VK_STRUCTURE_TYPE_RENDERING_INFO,
                .renderArea = { { 0, 0 }, extent2 },
                .layerCount = 1,
                .colorAttachmentCount = 1,
                .pColorAttachments = &color_attachment,
                .pDepthAttachment = &depth_attachment
            };

            vkCmdBeginRendering(cmd, &render_info);
        }

        // Set dynamic viewport and scissor
        {
            VkViewport viewport = {
                .width    = extent2.width,
                .height   = extent2.height,
                .minDepth = 0.0f,
                .maxDepth = 1.0f,
            };
            vkCmdSetViewport(cmd, 0, 1, &viewport);

            VkRect2D scissor = {
                .extent = extent2,
            };
            vkCmdSetScissor(cmd, 0, 1, &scissor);
        }

        // Draw the cube
        {
            vkCmdBindPipeline(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, vkpl);
            vkCmdBindDescriptorSets(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, vklayout, 0, 1,
                                    &vksets[f], 0, 0);
            VkDeviceSize offset = 0;
            vkCmdBindVertexBuffers(cmd, 0, 1, &vkvbuf, &offset);
            vkCmdBindIndexBuffer(cmd, vkibuf, 0, VK_INDEX_TYPE_UINT16);
            vkCmdDrawIndexed(cmd, COUNTOF(idata), 1, 0, 0, 0);
        }

        // End dynamic rendering
        vkCmdEndRendering(cmd);

        // Transition swapchain image: color attachment -> present
        {
            VkImageMemoryBarrier2 barrier = {
                .sType               = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER_2,
                .srcStageMask        = VK_PIPELINE_STAGE_2_COLOR_ATTACHMENT_OUTPUT_BIT,
                .srcAccessMask       = VK_ACCESS_2_COLOR_ATTACHMENT_WRITE_BIT,
                .dstStageMask        = VK_PIPELINE_STAGE_2_BOTTOM_OF_PIPE_BIT,
                .dstAccessMask       = 0,
                .oldLayout           = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
                .newLayout           = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR,
                .srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
                .dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
                .image               = swapchain_images[img_idx],
                .subresourceRange    = (VkImageSubresourceRange){
                    .aspectMask     = VK_IMAGE_ASPECT_COLOR_BIT,
                    .baseMipLevel   = 0,
                    .levelCount     = 1,
                    .baseArrayLayer = 0,
                    .layerCount     = 1
                }
            };

            VkDependencyInfo dep_info = {
                .sType                   = VK_STRUCTURE_TYPE_DEPENDENCY_INFO,
                .imageMemoryBarrierCount = 1,
                .pImageMemoryBarriers    = &barrier
            };
            vkCmdPipelineBarrier2(cmd, &dep_info);
        }

        // Done recording commands
        vkEndCommandBuffer(cmd);

        // Wait for these semaphores before swapping
        VkSemaphore wait_sems[]   = { vk_image_available_sems[f] };

        // Signal these semaphores after swapping
        VkSemaphore signal_sems[] = { vk_render_finished_sems[img_idx] };

        // Where to wait for wait_sems
        VkPipelineStageFlags wait_stages[] = {
            VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT
        };

        // Submit
        {
            VkSubmitInfo submit_info = {
                .sType                = VK_STRUCTURE_TYPE_SUBMIT_INFO,
                .waitSemaphoreCount   = COUNTOF(wait_sems),
                .pWaitSemaphores      = wait_sems,
                .pWaitDstStageMask    = wait_stages,
                .commandBufferCount   = 1,
                .pCommandBuffers      = &cmd,
                .signalSemaphoreCount = COUNTOF(signal_sems),
                .pSignalSemaphores    = signal_sems
            };
            vkQueueSubmit(vkq, 1, &submit_info, vk_in_flight_fences[f]);
        }

        // Present
        {
            VkPresentInfoKHR present_info = {
                .sType              = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR,
                .waitSemaphoreCount = COUNTOF(signal_sems),
                .pWaitSemaphores    = signal_sems,
                .swapchainCount     = 1,
                .pSwapchains        = &vkswapchain,
                .pImageIndices      = &img_idx
            };
            VkResult vkr = vkQueuePresentKHR(vkq, &present_info);
            if (vkr == VK_ERROR_OUT_OF_DATE_KHR || vkr == VK_SUBOPTIMAL_KHR) {
                swapchain_dirty = true;
            }
        }

        f = (f + 1) % max_frames_in_flight;
    }

    // the end is never the end is never the end is never the end is never the end is never the end

    return 0;
}

