set(SOURCES
    main.cpp
)

add_executable(vulkanBaseUnitTests
    ${SOURCES}
)

add_coverage(vulkanBaseUnitTests)

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