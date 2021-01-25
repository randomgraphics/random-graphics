# build glfw
option(GLFW_BUILD_EXAMPLES "" OFF)
option(GLFW_BUILD_TESTS "" OFF)
option(GLFW_BUILD_DOCS "" OFF)
option(GLFW_INSTALL "" OFF)
add_subdirectory(glfw)

# build unit test
add_executable(unit-test unit-test.cpp)
if (OPENGL_FOUND)
    target_compile_definitions(unit-test PRIVATE HAS_OPENGL=1)
endif()
target_link_libraries(unit-test PRIVATE random-graphics)

function(add_glfw_app target source)
    add_executable(${target} ${source})
    target_link_libraries(${target} random-graphics glfw)
endfunction()

function(add_qt_app target source)
    if (Qt6Widgets_FOUND)
        add_executable(${target} ${source})
        target_link_libraries(${target} random-graphics Qt::Widgets)
    elseif (Qt5Widgets_FOUND)
        add_executable(${target} ${source})
        target_link_libraries(${target} random-graphics Qt5::Widgets)
    endif()
endfunction()

# opengl test app
if (OPENGL_FOUND)
    add_glfw_app(opengl-test opengl-test.cpp)
    add_qt_app(qopengl-test qopengl-test.cpp)
endif()

if (VULKAN_FOUND)
    # TODO: add vulkan test app
endif()
