cmake_minimum_required(VERSION 3.9)

project(random-graphics)

option(RG_BUILD_STATIC "build as static library" ON)
option(RG_BUILD_TEST "build tests" ON)
option(RG_BUILD_SAMPLE "build sample apps" ON)
option(RG_BUILD_OPENGL "build opengl support" ON)
option(RG_BUILD_VULKAN "build vulkan support" ON)

# search for dependencies
if (RG_BUILD_OPENGL)
    find_package(OpenGL)
endif()

if (RG_BUILD_VULKAN)
    find_package(Vulkan)
endif()

# build glfw only when build test and/or sample folder
if (RG_BUILD_TEST OR RG_BUILD_SAMPLE)
    option(GLFW_BUILD_EXAMPLES "" OFF)
    option(GLFW_BUILD_TESTS "" OFF)
    option(GLFW_BUILD_DOCS "" OFF)
    option(GLFW_INSTALL "" OFF)
    add_subdirectory(3rdparty/glfw)
endif()

# enable strict warning checks for all targets (don't care about 3rdparty targets)
# choose to no use target_compile_options(), cause if we don't want these warning settings
# got propagated out of the project (by making them PUBLIC), nor want to redefine
# them repeatedly for all internal targets (by making them PRIVATE)
if (MSVC)
    add_compile_options(/W4 /WX)
elseif (GCC)
    add_compile_options(-Wall -Wextra -Werror)
endif()

add_subdirectory(src)

if (RG_BUILD_TEST)
    add_subdirectory(test)
endif()

if (RG_BUILD_SAMPLE)
    add_subdirectory(sample)
endif()
