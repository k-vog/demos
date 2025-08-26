#include "yuvbench.hh"

#include <vulkan/vulkan.h>

typedef struct VulkanContext VulkanContext;
struct VulkanContext
{
  VkInstance inst;
};

static void vk_check(VkResult result, const char* str)
{
  if (result != VK_SUCCESS) {
    const char* err_str = "(unknown)";
    switch (result) {
#define BIND_ERROR(errcode) case errcode: err_str = #errcode; break
      BIND_ERROR(VK_ERROR_DEVICE_LOST);
      BIND_ERROR(VK_ERROR_EXTENSION_NOT_PRESENT);
      BIND_ERROR(VK_ERROR_FEATURE_NOT_PRESENT);
      BIND_ERROR(VK_ERROR_FORMAT_NOT_SUPPORTED);
      BIND_ERROR(VK_ERROR_FRAGMENTED_POOL);
      BIND_ERROR(VK_ERROR_INCOMPATIBLE_DRIVER);
      BIND_ERROR(VK_ERROR_INITIALIZATION_FAILED);
      BIND_ERROR(VK_ERROR_LAYER_NOT_PRESENT);
      BIND_ERROR(VK_ERROR_MEMORY_MAP_FAILED);
      BIND_ERROR(VK_ERROR_OUT_OF_DEVICE_MEMORY);
      BIND_ERROR(VK_ERROR_OUT_OF_HOST_MEMORY);
      BIND_ERROR(VK_ERROR_TOO_MANY_OBJECTS);
      BIND_ERROR(VK_EVENT_RESET);
      BIND_ERROR(VK_EVENT_SET);
      BIND_ERROR(VK_INCOMPLETE);
      BIND_ERROR(VK_NOT_READY);
      BIND_ERROR(VK_TIMEOUT);
#undef BIND_ERROR
      default: r_assert(0);
    }
    printf("Vulkan error: %s %s\n", err_str, str);
    __builtin_trap();
  }
}

void yuv_vulkan_create(Context* ctx)
{
  VulkanContext* vk = MemAllocZ<VulkanContext>(1);

  VkApplicationInfo app_info = {
    .sType      = VK_STRUCTURE_TYPE_APPLICATION_INFO,
    .apiVersion = VK_API_VERSION_1_0,
  };
  VkInstanceCreateInfo create_info = {
    .sType            = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO,
    .pApplicationInfo = &app_info,
  };
  VkResult result = vkCreateInstance(&create_info, NULL, &vk->inst);
  vk_check(result, "vkCreateInstance");

  ctx->impl = vk;
}

void yuv_vulkan_process(Context* ctx)
{
}

void yuv_vulkan_destroy(Context* ctx)
{
  VulkanContext* vk = (VulkanContext*)ctx->impl;
  free(vk);
}
