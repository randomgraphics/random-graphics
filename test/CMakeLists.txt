# build unit test
add_executable(unit-test unit-test.cpp)
if (OPENGL_FOUND)
    target_compile_definitions(unit-test PRIVATE HAS_OPENGL=1)
endif()
target_link_libraries(unit-test PRIVATE random-graphics)

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

# search for Qt5/6
find_package(Qt5 COMPONENTS Widgets)
find_package(Qt6 COMPONENTS Widgets)

# opengl test app
if (OPENGL_FOUND)
    add_glfw_app(opengl-test opengl-test.cpp)
    add_qt_app(qopengl-test qopengl-test.cpp)
endif()

if (VULKAN_FOUND)
    # TODO: add vulkan test app
endif()
