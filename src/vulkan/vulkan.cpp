#define VOLK_IMPLEMENTATION
#include "rg/vulkan.h"
#include <unordered_map>

// ---------------------------------------------------------------------------------------------------------------------
//
const char * RG_API rg::vk::VkResultToString(VkResult error) {
    static const std::unordered_map<VkResult, const char *> table {
        std::make_pair(VK_SUCCESS, "VK_SUCCESS"),
        std::make_pair(VK_NOT_READY, "VK_NOT_READY"),
        std::make_pair(VK_TIMEOUT, "VK_TIMEOUT"),
        std::make_pair(VK_EVENT_SET, "VK_EVENT_SET"),
        std::make_pair(VK_EVENT_RESET, "VK_EVENT_RESET"),
        std::make_pair(VK_INCOMPLETE, "VK_INCOMPLETE"),
        std::make_pair(VK_ERROR_OUT_OF_HOST_MEMORY, "VK_ERROR_OUT_OF_HOST_MEMORY"),
        std::make_pair(VK_ERROR_OUT_OF_DEVICE_MEMORY, "VK_ERROR_OUT_OF_DEVICE_MEMORY"),
        std::make_pair(VK_ERROR_INITIALIZATION_FAILED, "VK_ERROR_INITIALIZATION_FAILED"),
        std::make_pair(VK_ERROR_DEVICE_LOST, "VK_ERROR_DEVICE_LOST"),
        std::make_pair(VK_ERROR_MEMORY_MAP_FAILED, "VK_ERROR_MEMORY_MAP_FAILED"),
        std::make_pair(VK_ERROR_LAYER_NOT_PRESENT, "VK_ERROR_LAYER_NOT_PRESENT"),
        std::make_pair(VK_ERROR_EXTENSION_NOT_PRESENT, "VK_ERROR_EXTENSION_NOT_PRESENT"),
        std::make_pair(VK_ERROR_FEATURE_NOT_PRESENT, "VK_ERROR_FEATURE_NOT_PRESENT"),
        std::make_pair(VK_ERROR_INCOMPATIBLE_DRIVER, "VK_ERROR_INCOMPATIBLE_DRIVER"),
        std::make_pair(VK_ERROR_TOO_MANY_OBJECTS, "VK_ERROR_TOO_MANY_OBJECTS"),
        std::make_pair(VK_ERROR_FORMAT_NOT_SUPPORTED, "VK_ERROR_FORMAT_NOT_SUPPORTED"),
        std::make_pair(VK_ERROR_FRAGMENTED_POOL, "VK_ERROR_FRAGMENTED_POOL"),
        std::make_pair(VK_ERROR_SURFACE_LOST_KHR, "VK_ERROR_SURFACE_LOST_KHR"),
        std::make_pair(VK_ERROR_NATIVE_WINDOW_IN_USE_KHR, "VK_ERROR_NATIVE_WINDOW_IN_USE_KHR"),
        std::make_pair(VK_SUBOPTIMAL_KHR, "VK_SUBOPTIMAL_KHR"),
        std::make_pair(VK_ERROR_OUT_OF_DATE_KHR, "VK_ERROR_OUT_OF_DATE_KHR"),
        std::make_pair(VK_ERROR_INCOMPATIBLE_DISPLAY_KHR, "VK_ERROR_INCOMPATIBLE_DISPLAY_KHR"),
        std::make_pair(VK_ERROR_VALIDATION_FAILED_EXT, "VK_ERROR_VALIDATION_FAILED_EXT"),
        std::make_pair(VK_ERROR_INVALID_SHADER_NV, "VK_ERROR_INVALID_SHADER_NV"),
    };
    auto iter = table.find(error);
    if (iter != table.end()) return iter->second;
    return rg::formatstr("Unrecognized VkResult 0x%08X", error);
}
