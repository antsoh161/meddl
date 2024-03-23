project(MeddlUnitTests)

file(GLOB SOURCES "vk/*.cpp")
add_executable(MeddlVkTests main.cpp ${SOURCES})
target_link_libraries(MeddlVkTests Meddl GTest::gtest_main)
add_test(NAME MeddlVkTests COMMAND MeddlVkTests)

# Bring the test shaders 
configure_file(${CMAKE_CURRENT_SOURCE_DIR}/vk/shader.vert ${CMAKE_CURRENT_BINARY_DIR}/shader.vert COPYONLY)
configure_file(${CMAKE_CURRENT_SOURCE_DIR}/vk/shader.frag ${CMAKE_CURRENT_BINARY_DIR}/shader.frag COPYONLY)
