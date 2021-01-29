# helper function to build glfw apps
function(add_glfw_app target source)
    add_executable(${target} ${source})
    target_link_libraries(${target} random-graphics glfw)
endfunction()

# helper function to build QT apps
find_package(Qt5 COMPONENTS Widgets)
find_package(Qt6 COMPONENTS Widgets)
function(add_qt_app target source)
    if (Qt6Widgets_FOUND)
        add_executable(${target} ${source})
        target_link_libraries(${target} random-graphics Qt::Widgets)
    elseif (Qt5Widgets_FOUND)
        add_executable(${target} ${source})
        target_link_libraries(${target} random-graphics Qt5::Widgets)
    endif()
endfunction()

# opengl samples
if (OPENGL_FOUND)
    add_glfw_app(opengl-hello opengl-hello.cpp)
    add_qt_app(qopengl-hello qopengl-hello.cpp)
endif()

# vulkan samples
if (VULKAN_FOUND)
    add_glfw_app(vulkan-hello vulkan-hello.cpp)
endif()
