cmake_minimum_required(VERSION 3.7)
set(CMAKE_CXX_STANDARD 11)

project(myVulkan LANGUAGES CXX)

SET(CMAKE_MODULE_PATH
    ${CMAKE_MODULE_PATH}
    ${CMAKE_SOURCE_DIR}/cmake
)

include(GNUInstallDirs)

add_subdirectory(externals)

if(MSVC)
    add_compile_options( /W4 /WX /MP )
elseif()
    add_compile_options( -W -Wall -Werror )
endif()

add_subdirectory(vulkan)
add_subdirectory(model_loader)
add_subdirectory(particles)
