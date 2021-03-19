# build unit test
add_executable(unit-test unit-test.cpp)
target_compile_definitions(unit-test PRIVATE TEST_FOLDER="${CMAKE_CURRENT_SOURCE_DIR}")
if (OPENGL_FOUND)
    target_compile_definitions(unit-test PRIVATE HAS_OPENGL=1)
endif()
target_link_libraries(unit-test PRIVATE random-graphics)
