find_package(Vulkan REQUIRED)

set(VULKAN_SOURCES
    include/basicrenderer.h
    include/swapchain.h
    include/vulkanhelper.h
    include/debug.h
    include/device.h
    include/shader.h
    include/descriptorset.h
    include/pipeline.h
    include/renderpass.h
    include/framebuffer.h
    include/vertexbuffer.h
    include/texture.h
    include/mesh.h
    include/handles.h
    include/window.h
    include/buffer.h
    src/basicrenderer.cpp
    src/swapchain.cpp
    src/vulkanhelper.cpp
    src/debug.cpp
    src/device.cpp
    src/shader.cpp    
    src/descriptorset.cpp    
    src/pipeline.cpp
    src/renderpass.cpp    
    src/framebuffer.cpp
    src/vertexbuffer.cpp
    src/texture.cpp
    src/mesh.cpp
    src/window.cpp
    src/buffer.cpp
)

set(UTILS_SOURCES
    utils/hasher.h
    utils/glm.h
    utils/timer.h
    utils/hasher.h
    utils/scopedtimelog.h
    utils/scopedtimelog.cpp
    utils/statistics.h
    utils/statistics.cpp
    utils/arcball_camera.h
    utils/arcball_camera.cpp
    utils/flythrough_camera.h
    utils/flythrough_camera.cpp
)

include_directories(${CMAKE_SOURCE_DIR}/externals/stb)
include_directories(${CMAKE_SOURCE_DIR}/externals/tiny_obj_loader)

source_group("utils" FILES ${UTILS_SOURCES})

add_library(vulkanBase
    ${VULKAN_SOURCES}
    ${UTILS_SOURCES}
)

target_link_libraries(vulkanBase
    PRIVATE
        glfw
        glm
    PUBLIC
        Vulkan::Vulkan
)

target_include_directories(vulkanBase
    PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/utils>
        $<BUILD_INTERFACE:${CMAKE_SOURCE_DIR}/externals/tiny_obj_loader>
)
