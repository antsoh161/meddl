project(MeddlUnitTests)

file(GLOB SOURCES "vk/*.cpp")
add_executable(MeddlVkTests main.cpp ${SOURCES})
target_link_libraries(MeddlVkTests Meddl GTest::gtest_main)
add_test(NAME MeddlVkTests COMMAND MeddlVkTests)

