set(SOURCES
    main.cpp
    renderer.h
    renderer.cpp
)

add_resources()

add_executable(depth_of_field
    ${SOURCES}
    ${SHADERS}
    ${BINARY_SHADERS}
)

set_target_properties(depth_of_field PROPERTIES VS_DEBUGGER_WORKING_DIRECTORY "${CMAKE_RUNTIME_OUTPUT_DIRECTORY}")
set_property(TARGET depth_of_field PROPERTY FOLDER "examples")

target_link_libraries(depth_of_field
    PRIVATE
        vulkanBase
)

install(
  TARGETS depth_of_field
  RUNTIME DESTINATION "${CMAKE_INSTALL_BINDIR}"
)
