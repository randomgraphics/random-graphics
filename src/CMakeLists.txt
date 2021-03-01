set(sources
    01-base/base.cpp
    01-base/log.cpp
    01-base/image.cpp
)

set(includes ../inc)

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
    list(APPEND sources vulkan/vulkan.cpp)
    list(APPEND includes ${Vulkan_INCLUDE_DIRS})
endif()

add_library(random-graphics STATIC ${sources})

# this is the proper way to enable c++17 that ensure all projects depends on it will also have c++17 enabled.
target_compile_features(random-graphics PUBLIC cxx_std_17)

target_compile_definitions(random-graphics
    PUBLIC
        $<$<CONFIG:Debug>:RG_BUILD_DEBUG=1>
    PRIVATE
        RG_INTERNAL=1
)

target_include_directories(random-graphics PUBLIC ${includes})

target_link_libraries(random-graphics PUBLIC ${libs})
if (UNIX)
    target_link_libraries(random-graphics PUBLIC atomic dl)
endif()
