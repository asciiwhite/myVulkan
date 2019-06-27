function(add_example EXAMPLE_NAME EXAMPLE_SOURCES)

    set(MESH_DIR ${RESOURCE_SRC_DIR}/meshes)
    get_filename_component(FULL_MESH_DIR ${MESH_DIR} REALPATH)
    if (EXISTS ${FULL_MESH_DIR})
        file(COPY ${MESH_DIR} DESTINATION ${RESOURCE_DST_DIR})
        #message(STATUS "Copying mesh dir " ${FULL_MESH_DIR})
    endif()
    
    add_resources()

    add_executable(${EXAMPLE_NAME}
        ${EXAMPLE_SOURCES}
        ${SHADERS}
        ${BINARY_SHADERS}
    )

    set_target_properties(${EXAMPLE_NAME} PROPERTIES VS_DEBUGGER_WORKING_DIRECTORY "${CMAKE_RUNTIME_OUTPUT_DIRECTORY}")
    set_property(TARGET ${EXAMPLE_NAME} PROPERTY FOLDER "examples")
    target_link_libraries(${EXAMPLE_NAME} PRIVATE vulkanBase)
    
    if(MSVC)
        target_compile_options(${EXAMPLE_NAME} PRIVATE /W4 /WX /MP)
    elseif()
        target_compile_options(${EXAMPLE_NAME} PRIVATE -W -Wall -Werror -pedantic)
    endif()

    install(
      TARGETS ${EXAMPLE_NAME}
      RUNTIME DESTINATION "${CMAKE_INSTALL_BINDIR}"
    )

endfunction()