cmake_minimum_required(VERSION 3.25.1)
project(Meddl)

set(CMAKE_COLOR_DIAGNOSTICS ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

set(BUILD_TESTS OFF)
set(BUILD_EXAMPLES ON)
set(BUILD_SHARED_LIBS ON)

set(FETCHCONTENT_QUIET OFF) # Debug
if(BUILD_TESTS)
   enable_testing()
   add_subdirectory(third_party/catch2)
endif()
add_subdirectory(third_party/glfw)
add_subdirectory(third_party/glm)
set(SPDLOG_USE_STD_FORMAT ON CACHE BOOL "Use std::format instead of fmt" FORCE)
set(SPDLOG_ENABLE_THREAD_POOL ON CACHE BOOL "Enable thread pool for async logging" FORCE)
add_subdirectory(third_party/spdlog)

find_package(Vulkan REQUIRED)

# idk shaderc cmake seems ancient so pkgconfig..?
find_package(PkgConfig REQUIRED)
pkg_check_modules(Shaderc IMPORTED_TARGET shaderc)

file(GLOB_RECURSE srcs CONFIGURE_DEPENDS src/*.cpp)
# file(GLOB_RECURSE modules CONFIGURE_DEPENDS src/*.cppm)
file(GLOB_RECURSE hdrs CONFIGURE_DEPENDS include/*.h include/*.hpp)

add_library(Meddl)
target_sources(Meddl PUBLIC
      ${srcs}
   PUBLIC
      FILE_SET HEADERS
      BASE_DIRS ${PROJECT_SOURCE_DIR}/include
      FILES
         ${hdrs}
   # PUBLIC
   #    FILE_SET CXX_MODULES
   #    BASE_DIRS ${PROJECT_SOURCE_DIR}/mod
   #    FILES
   #       ${modules}
)

target_link_libraries(Meddl PUBLIC
                              glfw glm Vulkan::Vulkan PkgConfig::Shaderc pthread
                           spdlog::spdlog)
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
