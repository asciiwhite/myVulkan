cmake_minimum_required(VERSION 3.7)

project(myVulkanInstallTest VERSION 1.0.0 LANGUAGES CXX)

find_package(VulkanBase REQUIRED CONFIG)

add_executable(vulkanBaseInstallTest main.cpp)
target_link_libraries(vulkanBaseInstallTest PRIVATE VulkanBase::vulkanBase)

enable_testing()
add_test(NAME vulkanBaseInstallTest COMMAND vulkanBaseInstallTest)