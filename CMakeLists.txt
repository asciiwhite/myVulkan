cmake_minimum_required(VERSION 3.7)

project(myVulkan VERSION 1.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
SET(CMAKE_MODULE_PATH
    ${CMAKE_MODULE_PATH}
    ${CMAKE_SOURCE_DIR}/cmake
    ${CMAKE_SOURCE_DIR}/externals/cmake-codecov/cmake
)

include(HandleResources)
include(Examples)

find_package(codecov)
add_subdirectory(externals)

set(RESOURCE_SRC_DIR data)
set(RESOURCE_DST_DIR ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/${RESOURCE_SRC_DIR})

add_subdirectory(vulkan)
add_subdirectory(examples)

enable_testing()

list(APPEND LCOV_REMOVE_PATTERNS "'${CMAKE_SOURCE_DIR}/externals/*'")
coverage_evaluate()

# Installation
install(
  DIRECTORY ${RESOURCE_DST_DIR} DESTINATION ${CMAKE_INSTALL_BINDIR}
)

install(
    TARGETS
      vulkanBase
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
    ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
)
