cmake_minimum_required(VERSION 3.9)

project(random-graphics)

option(RG_BUILD_STATIC "build as static library" ON)
option(RG_BUILD_TEST "build tests" ON)

# search for dependencies
find_package(OpenGL)
find_package(Vulkan)
find_package(Qt5 COMPONENTS Widgets)
find_package(Qt6 COMPONENTS Widgets)

add_subdirectory(src)

if (RG_BUILD_TEST)
    add_subdirectory(test)
endif()
