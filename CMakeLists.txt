cmake_minimum_required(VERSION 3.7)
set(CMAKE_CXX_STANDARD 11)

project(myVulkan LANGUAGES CXX)

if (MSVC)
    add_compile_options(/MP)
endif()

set(GLFW_BUILD_TESTS False)
set(GLFW_BUILD_EXAMPLES False)
set(GLFW_BUILD_DOCS False)
add_subdirectory(externals/glfw)
include_directories(externals/glfw/include)
include_directories(externals/stb)
include_directories(externals/tiny_obj_loader)

find_package(GLM REQUIRED)
include_directories(${GLM_INCLUDE_DIRS})

find_package(Vulkan REQUIRED)

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
    src/vulkan/mesh.h
    src/vulkan/mesh.cpp
    src/vulkan/handles.h
)

set(SOURCES
    src/main.cpp
    src/utils/glm.h
    src/utils/timer.h
    src/utils/hasher.h
    src/utils/scopedtimelog.h
    src/utils/scopedtimelog.cpp
    src/utils/statistics.h
    src/utils/statistics.cpp
    src/simplerenderer.h
    src/simplerenderer.cpp
    src/window.h
    src/window.cpp
)

source_group("source" FILES ${SOURCES})
source_group("vulkan" FILES ${VULKAN_SOURCES})

set(RESOURCE_DIR data)
set(TEXTURE_DIR ${RESOURCE_DIR}/textures)
set(MESH_DIR ${RESOURCE_DIR}/meshes)

include(CompileShaders.cmake)


add_executable(${PROJECT_NAME}
    ${VULKAN_SOURCES}
    ${SOURCES}
    ${SHADERS}
    ${BINARY_SHADERS}
)

target_link_libraries(${PROJECT_NAME}
    glfw
    Vulkan::Vulkan
)

if(MSVC)
    target_compile_options(${PROJECT_NAME} PRIVATE /W4 /WX)
elseif()
    target_compile_options(${PROJECT_NAME} PRIVATE -W -Wall -Werror)
endif()

file(COPY ${TEXTURE_DIR} DESTINATION ${CMAKE_BINARY_DIR}/${RESOURCE_DIR})
file(COPY ${MESH_DIR} DESTINATION ${CMAKE_BINARY_DIR}/${RESOURCE_DIR})
