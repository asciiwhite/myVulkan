set(SOURCES
    main.cpp
    renderer.h
    renderer.cpp
)

set(RESOURCE_DIR data)

include(CompileShaders)

add_executable(game
    ${SOURCES}
    ${SHADERS}
    ${BINARY_SHADERS}
)

target_link_libraries(game
    PRIVATE
        glfw
        glm
        vulkanBase
)

install(
  TARGETS game
  RUNTIME DESTINATION "${CMAKE_INSTALL_BINDIR}"
)

install(
  DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/${RESOURCE_DIR}
  DESTINATION "${CMAKE_INSTALL_BINDIR}"
)