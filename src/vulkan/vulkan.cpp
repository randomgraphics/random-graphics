#define VOLK_IMPLEMENTATION
#include "rg/vulkan.h"
#include <vulkan/vulkan.hpp>

const char * RG_API rg::vulkan::VkResultToString(VkResult r) {
    thread_local static std::string s;
    s = ::vk::to_string((::vk::Result)r);
    return s.c_str();
}
