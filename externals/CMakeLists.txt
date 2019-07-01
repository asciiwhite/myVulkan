# glfw
set(GLFW_INSTALL OFF CACHE BOOL "" FORCE)
set(GLFW_BUILD_TESTS OFF CACHE BOOL "" FORCE)
set(GLFW_BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)
set(GLFW_BUILD_DOCS OFF CACHE BOOL "" FORCE)
add_subdirectory(glfw)
set_property(TARGET glfw PROPERTY FOLDER "externals")

# glm
set(GLM_INSTALL_ENABLE ON CACHE BOOL "" FORCE)
set(GLM_STATIC_LIBRARY_ENABLE ON CACHE BOOL "" FORCE)
add_subdirectory(glm)
set_property(TARGET glm_static uninstall PROPERTY FOLDER "externals")

# gtest
set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)
set(INSTALL_GTEST OFF CACHE BOOL "" FORCE)
set(INSTALL_GMOCK OFF CACHE BOOL "" FORCE)
add_subdirectory(googletest)
mark_as_advanced(
    BUILD_GMOCK BUILD_GTEST BUILD_SHARED_LIBS
    gmock_build_tests gtest_build_samples gtest_build_tests
    gtest_disable_pthreads gtest_force_shared_crt gtest_hide_internal_symbols
)
set_property(TARGET gmock gmock_main gtest gtest_main PROPERTY FOLDER "externals")

# imgui
add_library(imgui STATIC
    imgui/imgui.cpp
    imgui/imgui.h
    imgui/imgui_draw.cpp
    imgui/imgui_widgets.cpp
    imgui/imgui_internal.h
    imgui/imstb_rectpack.h
    imgui/imstb_textedit.h
    imgui/imstb_truetype.h
)
target_include_directories(imgui
    PUBLIC $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/imgui>
           $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}/imgui>
)
target_compile_definitions(imgui PUBLIC IMGUI_DISABLE_INCLUDE_IMCONFIG_H)
set_target_properties(imgui PROPERTIES FOLDER "externals")

install(FILES imgui/imgui.h DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/imgui)

# stb
add_library(stb INTERFACE)
target_include_directories(stb SYSTEM INTERFACE $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/stb>)

# tiny_obj_loader
add_library(tiny_obj_loader INTERFACE)
target_include_directories(tiny_obj_loader SYSTEM INTERFACE $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/tiny_obj_loader>)