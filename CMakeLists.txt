cmake_minimum_required(VERSION 3.9)

project(random-graphics)

option(RG_BUILD_STATIC "build as static library" ON)
option(RG_BUILD_TEST "build tests" ON)
option(RG_BUILD_SAMPLE "build sample apps" ON)

# search for dependencies
find_package(OpenGL)
find_package(Vulkan)

add_subdirectory(3rdparty)

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
