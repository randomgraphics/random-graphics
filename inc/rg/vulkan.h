#pragma once
#include "base.h"
#include "vulkan/volk.h"

#define RG_VKCHK(func, actionOnFailure) \
    if constexpr (true) { \
        auto result__ = (func); \
        if ((result__) != VK_SUCCESS) { \
            RG_LOGE("%s failed: %s", #func, rg::vk::VkResultToString(result__)); \
            actionOnFailure; \
        } \
    } else void(0)


namespace rg::vk {

/// convert VKResult to string
const char * RG_API VkResultToString(VkResult);

}