project(MeddlUnitTests)

file(GLOB SOURCES "*.cpp")
add_executable(vulkan_unit_tests ${SOURCES})
target_link_libraries(vulkan_unit_tests GTest::gtest)
add_test(NAME VulkanUnitTests COMMAND vulkan_unit_tests)

