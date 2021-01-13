cmake_minimum_required(VERSION 3.9)

project(random-graphics)

option(RG_BUILD_STATIC "build as static library" on)
option(RG_BUILD_TEST "build tests" on)

set(CMAKE_CXX_STANDARD 17) # todo: set it per target?

find_package(OpenGL)

add_subdirectory(src)

if (RG_BUILD_TEST)
    add_subdirectory(test)
endif()