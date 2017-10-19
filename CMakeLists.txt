cmake_minimum_required(VERSION 3.7)

project(myVulkan)

if (MSVC)
    add_compile_options(/W3 /WX)
else()
    add_compile_options(/W /Wall /Werror)
endif()


if(MSVC)
    set(SDL2_DIR externals/SDL2-2.0.6)
endif()
find_package(SDL2 REQUIRED)
include_directories(${SDL2_INCLUDE_DIRS})

find_package(Vulkan REQUIRED)

include_directories(externals/stb)

set(VULKAN_SOURCES
    src/vulkan/basicrenderer.h
    src/vulkan/basicrenderer.cpp
    src/vulkan/swapchain.h
    src/vulkan/swapchain.cpp
    src/vulkan/vulkanhelper.h
    src/vulkan/vulkanhelper.cpp
    src/vulkan/debug.h
    src/vulkan/debug.cpp
    src/vulkan/device.h
    src/vulkan/device.cpp
    src/vulkan/shader.h
    src/vulkan/shader.cpp
    src/vulkan/descriptorset.h
    src/vulkan/descriptorset.cpp
    src/vulkan/pipeline.h
    src/vulkan/pipeline.cpp
    src/vulkan/renderpass.h
    src/vulkan/renderpass.cpp
    src/vulkan/framebuffer.h
    src/vulkan/framebuffer.cpp
    src/vulkan/vertexbuffer.h
    src/vulkan/vertexbuffer.cpp
    src/vulkan/texture.h
    src/vulkan/texture.cpp
)

set(SOURCES
    src/main.cpp
    src/simplerenderer.h
    src/simplerenderer.cpp
)

source_group("source" FILES ${SOURCES})
source_group("vulkan" FILES ${VULKAN_SOURCES})

set(RESOURCE_DIR data)
set(TEXTURE_DIR ${RESOURCE_DIR}/textures)

include(CompileShaders.cmake)


add_executable(${PROJECT_NAME}
    ${VULKAN_SOURCES}
    ${SOURCES}
    ${SHADERS}
    ${BINARY_SHADERS}
)

target_link_libraries(${PROJECT_NAME}
    ${SDL2_LIBRARIES}
    Vulkan::Vulkan
)

if(MSVC)
    target_compile_options(${PROJECT_NAME} PRIVATE "/MP")

    ADD_CUSTOM_COMMAND(
        TARGET ${PROJECT_NAME}
        POST_BUILD
        COMMAND ${CMAKE_COMMAND} ARGS -E copy_if_different ${SDL2_RUNTIME_LIBRARIES} ${CMAKE_CURRENT_BINARY_DIR}
    )
endif()

file(COPY ${TEXTURE_DIR} DESTINATION ${CMAKE_BINARY_DIR}/${RESOURCE_DIR})
