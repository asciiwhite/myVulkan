set(SOURCES
    main.cpp
    simplerenderer.h
    simplerenderer.cpp
    texturearraymesh.h
    texturearraymesh.cpp
)

set(RESOURCE_DIR data)
set(MESH_DIR ${RESOURCE_DIR}/meshes)

include(CompileShaders)

add_executable(texture_array
    ${SOURCES}
    ${SHADERS}
    ${BINARY_SHADERS}
)

target_link_libraries(texture_array
    PRIVATE
        glfw
        glm
        vulkanBase
)

file(COPY ${MESH_DIR} DESTINATION ${CMAKE_CURRENT_BINARY_DIR}/${RESOURCE_DIR})

install(
  TARGETS texture_array
  RUNTIME DESTINATION "${CMAKE_INSTALL_BINDIR}"
)

#install(
#  DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/${RESOURCE_DIR}
#  DESTINATION "${CMAKE_INSTALL_BINDIR}"
#)