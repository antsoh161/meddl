project(MeddlUnitTests)

include(FetchContent)

FetchContent_Declare(
  googletest
  GIT_REPOSITORY https://github.com/google/googletest.git
  GIT_TAG release-1.11.0  
)

FetchContent_MakeAvailable(googletest)

file(GLOB SOURCES "*.cpp")
add_executable(vulkan_unit_tests ${SOURCES})
target_link_libraries(vulkan_unit_tests GTest::gtest_main)
