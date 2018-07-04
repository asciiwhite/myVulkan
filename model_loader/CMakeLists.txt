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

target_link_libraries(model_loader
    PRIVATE
        vulkanBase
)

file(COPY ${TEXTURE_DIR} DESTINATION ${CMAKE_CURRENT_BINARY_DIR}/${RESOURCE_DIR})
file(COPY ${MESH_DIR} DESTINATION ${CMAKE_CURRENT_BINARY_DIR}/${RESOURCE_DIR})

install(
  TARGETS model_loader
  RUNTIME DESTINATION "${CMAKE_INSTALL_BINDIR}"
)

#install(
#  DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/${RESOURCE_DIR}
#  DESTINATION "${CMAKE_INSTALL_BINDIR}"
#)