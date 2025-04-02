project(MeddlUnitTests)

# file(GLOB vk_srcs "vk/*.cpp")
# add_executable(MeddlVkTests vk/main.cpp ${vk_srcs})
# target_link_libraries(MeddlVkTests Meddl Catch2::Catch2WithMain)
# add_test(NAME MeddlVkTests COMMAND MeddlVkTests)
# catch_discover_tests(MeddlVkTests)

# Bring the test shaders
configure_file(${CMAKE_CURRENT_SOURCE_DIR}/vk/shader.vert ${CMAKE_CURRENT_BINARY_DIR}/shader.vert COPYONLY)
configure_file(${CMAKE_CURRENT_SOURCE_DIR}/vk/shader.frag ${CMAKE_CURRENT_BINARY_DIR}/shader.frag COPYONLY)

file(GLOB core_srcs "core/*.cpp")
add_executable(MeddlCoreTest ${core_srcs})
target_link_libraries(MeddlCoreTest Meddl Catch2::Catch2WithMain)
add_test(NAME MeddlCoreTest COMMAND MeddlCoreTest)
