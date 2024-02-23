cmake_minimum_required(VERSION 3.25.1)
project(Meddl)

set(CMAKE_COLOR_DIAGNOSTICS ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

set(BUILD_TESTS ON)
set(BUILD_EXAMPLES ON)
set(BUILD_SHARED_LIBS ON)

set(FETCHCONTENT_QUIET OFF) # Debug
if(BUILD_TESTS)
add_subdirectory(third_party/googletest)
endif()
add_subdirectory(third_party/glfw)
add_subdirectory(third_party/glm)

find_package(Vulkan REQUIRED)

# idk shaderc cmake seems ancient so pkgconfig..?
find_package(PkgConfig REQUIRED)
pkg_check_modules(Shaderc IMPORTED_TARGET shaderc)

file(GLOB SOURCES "src/**/*.cpp" "src/*.cpp")
file(GLOB HEADERS "include/**/*.h" "include/**/*.hpp")

add_library(Meddl ${SOURCES})
target_include_directories(
  Meddl PUBLIC ${glfw_SOURCE_DIR}/include ${glm_SOURCE_DIR}
               ${VULKAN_INCLUDE_DIRS} ${CMAKE_CURRENT_SOURCE_DIR}/include) 

target_link_libraries(Meddl PUBLIC glfw Vulkan::Vulkan PkgConfig::Shaderc pthread)
target_compile_definitions(Meddl PUBLIC GLFW_INCLUDE_VULKAN)
target_compile_options(
  Meddl
  PRIVATE $<$<CONFIG:Release>:-O2>
  PRIVATE $<$<CONFIG:Debug>:-O0>)

if(BUILD_EXAMPLES)
   add_subdirectory(examples)
endif()

if(BUILD_TESTS)
  add_subdirectory(tests)
endif()
