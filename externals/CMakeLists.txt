# glfw
set(GLFW_INSTALL OFF CACHE BOOL "" FORCE)
set(GLFW_BUILD_TESTS OFF CACHE BOOL "" FORCE)
set(GLFW_BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)
set(GLFW_BUILD_DOCS OFF CACHE BOOL "" FORCE)
add_subdirectory(glfw)
set_property(TARGET glfw PROPERTY FOLDER "externals")

# glm
set(GLM_INSTALL_ENABLE OFF CACHE BOOL "" FORCE)
set(GLM_STATIC_LIBRARY_ENABLE ON CACHE BOOL "" FORCE)
add_subdirectory(glm)
set_property(TARGET uninstall PROPERTY FOLDER "externals")
set_property(TARGET glm_static PROPERTY FOLDER "externals")

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
set_property(TARGET gmock      PROPERTY FOLDER "externals")
set_property(TARGET gmock_main PROPERTY FOLDER "externals")
set_property(TARGET gtest      PROPERTY FOLDER "externals")
set_property(TARGET gtest_main PROPERTY FOLDER "externals")

# imgui
add_library(imgui
    imgui/imgui.cpp
    imgui/imgui.h
    imgui/imgui_draw.cpp
    imgui/imgui_widgets.cpp
    imgui/imgui_internal.h
    imgui/imconfig.h
    imgui/imstb_rectpack.h
    imgui/imstb_textedit.h
    imgui/imstb_truetype.h
)
target_include_directories(imgui PUBLIC imgui)
set_property(TARGET imgui PROPERTY FOLDER "externals")

# stb
get_filename_component(STB_INCLUDE_DIR ${CMAKE_CURRENT_LIST_FILE} DIRECTORY)
add_library(stb INTERFACE)
set_target_properties(stb PROPERTIES INTERFACE_INCLUDE_DIRECTORIES "${STB_INCLUDE_DIR}/stb")

# tiny_obj_loader
get_filename_component(TINY_OBJ_LOADER_INCLUDE_DIR ${CMAKE_CURRENT_LIST_FILE} DIRECTORY)
add_library(tiny_obj_loader INTERFACE)
set_target_properties(tiny_obj_loader PROPERTIES INTERFACE_INCLUDE_DIRECTORIES "${TINY_OBJ_LOADER_INCLUDE_DIR}/tiny_obj_loader")