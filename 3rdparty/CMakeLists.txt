# build glfw
option(GLFW_BUILD_EXAMPLES "" OFF)
option(GLFW_BUILD_TESTS "" OFF)
option(GLFW_BUILD_DOCS "" OFF)
option(GLFW_INSTALL "" OFF)
add_subdirectory(glfw)
set_target_properties(glfw PROPERTIES EXCLUDE_FROM_ALL TRUE) # build glfw only when it is referenced.