set(GLFW_INSTALL Off)
set(GLFW_BUILD_TESTS False)
set(GLFW_BUILD_EXAMPLES False)
set(GLFW_BUILD_DOCS False)
add_subdirectory(glfw)

set(GLM_INSTALL_ENABLE Off)
add_subdirectory(glm)