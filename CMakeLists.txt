cmake_minimum_required(VERSION 3.7)
set(CMAKE_CXX_STANDARD 11)

project(myVulkan LANGUAGES CXX)

SET(CMAKE_MODULE_PATH
    ${CMAKE_MODULE_PATH}
    ${CMAKE_SOURCE_DIR}/cmake
    ${CMAKE_SOURCE_DIR}/externals/cmake-codecov/cmake
)

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
add_subdirectory(texture_array)
add_subdirectory(particles)

list(APPEND LCOV_REMOVE_PATTERNS "'${CMAKE_SOURCE_DIR}/externals/*'")
coverage_evaluate()