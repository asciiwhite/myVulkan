set(SOURCES
    main.cpp
    renderer.h
    renderer.cpp
)

add_resources()

add_executable(bloom
    ${SOURCES}
    ${SHADERS}
    ${BINARY_SHADERS}
)

set_target_properties(bloom PROPERTIES VS_DEBUGGER_WORKING_DIRECTORY "${CMAKE_RUNTIME_OUTPUT_DIRECTORY}")
set_property(TARGET bloom PROPERTY FOLDER "examples")
#add_coverage(particles)

target_link_libraries(bloom
    PRIVATE
        vulkanBase
)

install(
  TARGETS bloom
  RUNTIME DESTINATION "${CMAKE_INSTALL_BINDIR}"
)
