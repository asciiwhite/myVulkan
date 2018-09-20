set(SOURCES
    main.cpp
    simplerenderer.h
    simplerenderer.cpp
    texturearraymesh.h
    texturearraymesh.cpp
)

add_resources()

add_executable(texture_array
    ${SOURCES}
    ${SHADERS}
    ${BINARY_SHADERS}
)

set_target_properties(texture_array PROPERTIES VS_DEBUGGER_WORKING_DIRECTORY "${CMAKE_RUNTIME_OUTPUT_DIRECTORY}")

target_link_libraries(texture_array
    PRIVATE
        vulkanBase
)

install(
  TARGETS texture_array
  RUNTIME DESTINATION "${CMAKE_INSTALL_BINDIR}"
)

#install(
#  DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/${RESOURCE_DIR}
#  DESTINATION "${CMAKE_INSTALL_BINDIR}"
#)