cmake_minimum_required(VERSION 3.7)

project(myVulkan LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
set(RESOURCE_SRC_DIR data)
set(RESOURCE_DST_DIR ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/${RESOURCE_SRC_DIR})

SET(CMAKE_MODULE_PATH
    ${CMAKE_MODULE_PATH}
    ${CMAKE_SOURCE_DIR}/cmake
    ${CMAKE_SOURCE_DIR}/externals/cmake-codecov/cmake
)

include(HandleResources)
include(GNUInstallDirs)

find_package(codecov)

add_subdirectory(externals)

if(MSVC)
    add_compile_options( /W4 /WX /MP )
elseif()
    add_compile_options( -W -Wall -Werror -pedantic)
endif()

enable_testing()

add_subdirectory(vulkan)
add_subdirectory(model_loader)
add_subdirectory(particles)
add_subdirectory(bloom)

install(
  DIRECTORY ${RESOURCE_DST_DIR}
  DESTINATION ${CMAKE_INSTALL_BINDIR}
)

list(APPEND LCOV_REMOVE_PATTERNS "'${CMAKE_SOURCE_DIR}/externals/*'")
coverage_evaluate()