if (RG_BUILD_STATIC)
    set(type STATIC)
else()
    set(type SHARED)
endif()

set(sources 01-base.cpp)

if (OPENGL_FOUND)
    list(APPEND sources 02-opengl.cpp)
endif()

add_library(random-graphics ${type} ${sources})
target_include_directories(random-graphics PUBLIC ../inc)
