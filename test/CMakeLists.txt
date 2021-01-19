add_executable(unit-test unit-test.cpp)
target_link_libraries(unit-test random-graphics)

find_package(Qt6 COMPONENTS Widgets)
if (Qt6Widgets_FOUND)
    add_executable(gldemo gldemo.cpp)
    target_link_libraries(gldemo random-graphics Qt::Widgets)
endif()
