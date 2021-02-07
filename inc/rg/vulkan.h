#pragma once

#include "base.h"

// if vulkan headers are already include, then we'll skip including volk.h
#if !defined(VULKAN_H_) || defined(VK_NO_PROTOTYPES)
#if RG_MSWIN && !defined(VK_USE_PLATFORM_WIN32_KHR)
#define VK_USE_PLATFORM_WIN32_KHR
#endif
#include <volk/volk.h>
#endif

#define RG_VKCHK(func, actionOnFailure) \
    if constexpr (true) { \
        auto result__ = (func); \
        /* there are a few positive success code other than VK_SUCCESS */ \
        if (result__ < 0) { \
            RG_LOGE("%s failed: %s", #func, ::rg::vulkan::VkResultToString(result__)); \
            actionOnFailure; \
        } \
    } else void(0)

namespace rg::vulkan {

/// convert VKResult to string
const char * RG_API VkResultToString(VkResult);

}