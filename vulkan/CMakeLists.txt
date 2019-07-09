find_package(Vulkan REQUIRED)

if (Vulkan_LIBRARY)
    message(STATUS "Found Vulkan library: " ${Vulkan_LIBRARY})
else()    
    message(FATAL_ERROR "Could not find Vulkan SDK!")
endif()

set(VULKAN_SOURCES
    include/basicrenderer.h
    include/swapchain.h
    include/vulkanhelper.h
    include/debug.h
    include/barrier.h
    include/device.h
    include/deviceref.h
    include/devicedestroy.h
    include/shader.h
    include/descriptorset.h
    include/graphicspipeline.h
    include/vertexbuffer.h
    include/image.h
    include/imagebase.h
    include/imageview.h
    include/window.h
    include/buffer.h
    include/bufferbase.h
    include/resourcemanager.h
    include/imagepool.h
    src/basicrenderer.cpp
    src/swapchain.cpp
    src/vulkanhelper.cpp
    src/debug.cpp
    src/barrier.cpp
    src/device.cpp
    src/deviceref.cpp
    src/devicedestroy.cpp
    src/shader.cpp    
    src/descriptorset.cpp    
    src/graphicspipeline.cpp
    src/vertexbuffer.cpp
    src/image.cpp
    src/imagebase.cpp
    src/imageview.cpp
    src/window.cpp
    src/bufferbase.cpp
    src/imagepool.cpp
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
    utils/gui.h
    utils/gui.cpp
    utils/objfileloader.h
    utils/objfileloader.cpp
    utils/imageloader.h
    utils/imageloader.cpp
    utils/meshdescription.h
    utils/mesh.h
    utils/mesh.cpp
    utils/camerainputhandler.h
    utils/camerainputhandler.cpp
    utils/mouseinputhandler.h
    utils/mouseinputhandler.cpp
)

source_group("utils" FILES ${UTILS_SOURCES})

add_resources()

add_library(vulkanBase
    ${VULKAN_SOURCES}
    ${UTILS_SOURCES}
    ${SHADERS}
    ${BINARY_SHADERS}
)

add_coverage(vulkanBase)

target_link_libraries(vulkanBase
    PUBLIC
        imgui
        glm
        Vulkan::Vulkan
    PRIVATE
        glfw
        tiny_obj_loader
        stb
)

target_include_directories(vulkanBase
    PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/utils>
        $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}/vulkanBase>
        $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}/vulkanBase/utils>
)

add_library(VulkanBase::vulkanBase ALIAS vulkanBase)

target_compile_features(vulkanBase PUBLIC cxx_std_17)

if(MSVC)
    target_compile_options(vulkanBase PRIVATE /W4 /WX /MP)
elseif()
    target_compile_options(vulkanBase PRIVATE -W -Wall -Werror -pedantic)
endif()

add_subdirectory(test)

# Installation
set(INSTALL_CONFIGDIR "${CMAKE_INSTALL_LIBDIR}/cmake")

install(
    DIRECTORY ${RESOURCE_DST_DIR} DESTINATION "${CMAKE_INSTALL_BINDIR}"
)

install(
    TARGETS vulkanBase imgui glm glfw stb tiny_obj_loader
    EXPORT vulkanBaseTargets
    ARCHIVE DESTINATION "${CMAKE_INSTALL_LIBDIR}"
    LIBRARY DESTINATION "${CMAKE_INSTALL_LIBDIR}"
    RUNTIME DESTINATION "${CMAKE_INSTALL_BINDIR}"
    INCLUDES DESTINATION "${CMAKE_INSTALL_INCLUDEDIR}"
)

install(DIRECTORY include/ DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/vulkanBase)
install(DIRECTORY utils DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/vulkanBase FILES_MATCHING PATTERN "*.h*")

install(EXPORT vulkanBaseTargets
    FILE vulkanBaseTargets.cmake
    NAMESPACE VulkanBase::
    DESTINATION ${INSTALL_CONFIGDIR})

# ConfigVersion file
include(CMakePackageConfigHelpers)
write_basic_package_version_file(
    ${CMAKE_CURRENT_BINARY_DIR}/vulkanBaseConfigVersion.cmake
    VERSION ${PROJECT_VERSION}
    COMPATIBILITY AnyNewerVersion
)

configure_package_config_file(
    ${CMAKE_SOURCE_DIR}/cmake/vulkanBaseConfig.cmake.in
    ${CMAKE_CURRENT_BINARY_DIR}/vulkanBaseConfig.cmake
    INSTALL_DESTINATION ${INSTALL_CONFIGDIR}
)

# Install all the helper files
install(
    FILES
        ${CMAKE_CURRENT_BINARY_DIR}/vulkanBaseConfig.cmake
        ${CMAKE_CURRENT_BINARY_DIR}/vulkanBaseConfigVersion.cmake
    DESTINATION ${INSTALL_CONFIGDIR}
)