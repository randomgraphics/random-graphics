if (RG_BUILD_STATIC)
    set(type STATIC)
    set(viz  PUBLIC)
else()
    set(type SHARED)
    set(viz  PRIVATE)
endif()

set(sources
    01-base/base.cpp
    01-base/log.cpp
    01-base/image.cpp
)

if (OPENGL_FOUND)
    list(APPEND sources 02-opengl/opengl.cpp)
    list(APPEND sources 02-opengl/glad/glad.c)
    if (WIN32)
        list(APPEND sources 02-opengl/glad/glad_wgl.c)
    endif()
    if (UNIX)
        list(APPEND sources 02-opengl/glad/glad_egl.c)
        list(APPEND sources 02-opengl/glad/glad_glx.c)
    endif()
    list(APPEND libs OpenGL::GL)
    if (TARGET OpenGL::EGL)
        list(APPEND libs OpenGL::EGL)
    endif()
endif()

if (VULKAN_FOUND)
    # TODO: add vulkan sources
endif()

add_library(random-graphics ${type} ${sources})

# this is the proper way to enable c++17 that ensure all projects depends on it will also have c++17 enabled.
target_compile_features(random-graphics PUBLIC cxx_std_17)

if (MSVC)
    target_compile_options(random-graphics PRIVATE /W4 /WX)
elseif (GCC)
    target_compile_options(random-graphics -Wall -Wextra -Werror)
endif()

target_compile_definitions(random-graphics
    PUBLIC
        $<$<BOOL:${RG_BUILD_STATIC}>:RG_BUILD_STATIC=1>
        $<$<CONFIG:Debug>:RG_BUILD_DEBUG=1>
    PRIVATE
        RG_INTERNAL=1
)

target_include_directories(random-graphics PUBLIC ../inc)

target_link_libraries(random-graphics ${viz} ${libs})
if (UNIX)
    target_link_libraries(random-graphics ${viz} atomic dl)
endif()
