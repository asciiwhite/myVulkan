set(SOURCES
    main.cpp
    simplerenderer.h
    simplerenderer.cpp
)

set(RESOURCE_DIR data)
set(TEXTURE_DIR ${RESOURCE_DIR}/textures)
set(MESH_DIR ${RESOURCE_DIR}/meshes)

include(CompileShaders)

add_executable(model_loader
    ${SOURCES}
    ${SHADERS}
    ${BINARY_SHADERS}
)

set_target_properties(model_loader PROPERTIES VS_DEBUGGER_WORKING_DIRECTORY "${CMAKE_RUNTIME_OUTPUT_DIRECTORY}")

target_link_libraries(model_loader
    PRIVATE
        vulkanBase
)

file(COPY ${TEXTURE_DIR} DESTINATION ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/${RESOURCE_DIR})
file(COPY ${MESH_DIR} DESTINATION ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/${RESOURCE_DIR})

install(
  TARGETS model_loader
  RUNTIME DESTINATION "${CMAKE_INSTALL_BINDIR}"
)

#install(
#  DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/${RESOURCE_DIR}
#  DESTINATION "${CMAKE_INSTALL_BINDIR}"
#)