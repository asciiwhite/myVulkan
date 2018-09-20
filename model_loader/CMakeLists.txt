set(SOURCES
    main.cpp
    simplerenderer.h
    simplerenderer.cpp
)

add_resources()

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

install(
  TARGETS model_loader
  RUNTIME DESTINATION "${CMAKE_INSTALL_BINDIR}"
)

#install(
#  DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/${RESOURCE_DIR}
#  DESTINATION "${CMAKE_INSTALL_BINDIR}"
#)