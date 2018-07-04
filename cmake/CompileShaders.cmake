find_program(GLSLANGVALIDATOR glslangValidator HINTS "$ENV{VULKAN_SDK}/bin")

if (NOT GLSLANGVALIDATOR)
    message(FATAL_ERROR "glslangValidator not found")
endif()

set(SHADER_DIR ${RESOURCE_DIR}/shaders)

file(GLOB SHADERS "${SHADER_DIR}/*.vert" "${SHADER_DIR}/*.frag" "${SHADER_DIR}/*.comp")
source_group("shaders" FILES ${SHADERS})

set(SHADER_OUTPUT_DIR ${CMAKE_CURRENT_BINARY_DIR}/${SHADER_DIR}/)
foreach(SHADER ${SHADERS})
    get_filename_component(FILENAME ${SHADER} NAME)
    set(BINARY_SHADER ${SHADER_OUTPUT_DIR}${FILENAME}.spv)
    add_custom_command(
        OUTPUT ${BINARY_SHADER}
        COMMAND ${CMAKE_COMMAND} -E make_directory ${SHADER_OUTPUT_DIR}
        COMMAND ${GLSLANGVALIDATOR} -V ${SHADER} -o ${BINARY_SHADER}
        DEPENDS ${SHADER}
        COMMENT "Building ${FILENAME}.spv"
    )
    list(APPEND BINARY_SHADERS ${BINARY_SHADER})
endforeach(SHADER)

source_group("binary_shaders" FILES ${BINARY_SHADERS})