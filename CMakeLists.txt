cmake_minimum_required(VERSION 3.25.1)
project(VulkanGameEngine)

set(CMAKE_EXPORT_COMPILE_COMMANDS True)
set(BUILD_SHARED_LIBS ON)
set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

set(FETCHCONTENT_QUIET OFF) # Debug
include(FetchContent)

FetchContent_Declare(
  glfw
  GIT_REPOSITORY https://github.com/glfw/glfw.git
  GIT_TAG 7482de6071d21db77a7236155da44c172a7f6c9e # 3.3.8
)
FetchContent_MakeAvailable(glfw)

FetchContent_Declare(
  glm
  GIT_REPOSITORY https://github.com/g-truc/glm.git
  GIT_TAG bf71a834948186f4097caa076cd2663c69a10e1e # 0.9.9.8
)
FetchContent_MakeAvailable(glm)

find_package(Vulkan REQUIRED)

file(GLOB SOURCES "src/*.cpp")
file(GLOB HEADERS "include/*.h")

add_executable(TestMeddl ${SOURCES})
target_link_libraries(TestMeddl PRIVATE glfw Vulkan::Vulkan)
target_include_directories(
  TestMeddl PRIVATE ${glfw_SOURCE_DIR}/include ${glm_SOURCE_DIR}
                    ${VULKAN_INCLUDE_DIRS} ${CMAKE_CURRENT_SOURCE_DIR}/include)

target_compile_options(
  TestMeddl
  PRIVATE $<$<CONFIG:Release>:-O2>
  PRIVATE $<$<CONFIG:Debug>:-O0>)

add_subdirectory(tests)
