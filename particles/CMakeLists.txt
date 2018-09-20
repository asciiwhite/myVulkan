set(SOURCES
    main.cpp
    renderer.h
    renderer.cpp
)

set(RESOURCE_DIR data)

include(CompileShaders)

add_executable(particles
    ${SOURCES}
    ${SHADERS}
    ${BINARY_SHADERS}
)

set_target_properties(particles PROPERTIES VS_DEBUGGER_WORKING_DIRECTORY "${CMAKE_RUNTIME_OUTPUT_DIRECTORY}")

#add_coverage(particles)

target_link_libraries(particles
    PRIVATE
        vulkanBase
)

install(
  TARGETS particles
  RUNTIME DESTINATION "${CMAKE_INSTALL_BINDIR}"
)

install(
  DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/${RESOURCE_DIR}
  DESTINATION "${CMAKE_INSTALL_BINDIR}"
)
