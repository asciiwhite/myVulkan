set(SOURCES
    main.cpp
)

add_executable(vulkanBaseUnitTests
    ${SOURCES}
)

target_link_libraries(vulkanBaseUnitTests
    PRIVATE
        vulkanBase
		gtest
		gtest_main
)

install(
  TARGETS vulkanBaseUnitTests
  RUNTIME DESTINATION "${CMAKE_INSTALL_BINDIR}"
)

add_test(NAME vulkanBaseUnitTests COMMAND vulkanBaseUnitTests)