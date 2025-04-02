cmake_minimum_required(VERSION 3.25.1)
project(Meddl)

set(CMAKE_COLOR_DIAGNOSTICS ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
set(CMAKE_CXX_STANDARD 26)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

set(BUILD_TESTS OFF)
set(BUILD_EXAMPLES ON)
set(BUILD_SHARED_LIBS ON)

if(MSVC)
   set(BUILD_SHARED_LIBS OFF)
    set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>")
   set(GLFW_BUILD_SHARED_LIBS OFF)
   set(SPDLOG_BUILD_SHARED OFF)
endif()

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
set(TINYGLTF_HEADER_ONLY ON CACHE INTERNAL "" FORCE)
set(TINYGLTF_INSTALL OFF CACHE INTERNAL "" FORCE)
add_subdirectory(third_party/tinygltf)

set(STDEXEC_BUILD_TESTS OFF)
add_subdirectory(third_party/stdexec)

find_package(Vulkan REQUIRED)
if(WIN32)
    add_subdirectory(third_party/shaderc)
    set(SHADERC_LIB shaderc_combined)
else()
    find_package(PkgConfig REQUIRED)
   pkg_check_modules(Shaderc IMPORTED_TARGET shaderc)
    set(SHADERC_LIB PkgConfig::Shaderc)
endif()

file(GLOB_RECURSE srcs CONFIGURE_DEPENDS src/*.cpp)
file(GLOB_RECURSE hdrs CONFIGURE_DEPENDS include/*.h include/*.hpp)

add_library(Meddl)
target_sources(Meddl PUBLIC
      ${srcs}
   PUBLIC
      FILE_SET HEADERS
      BASE_DIRS ${PROJECT_SOURCE_DIR}/include
      FILES
         ${hdrs}
)

target_link_libraries(Meddl PUBLIC
    glfw
    glm
    Vulkan::Vulkan
    # PkgConfig::Shaderc
    ${SHADERC_LIB}
    spdlog::spdlog
    tinygltf
    $<$<NOT:$<PLATFORM_ID:Windows>>:pthread>
)

target_compile_definitions(Meddl PUBLIC GLFW_INCLUDE_VULKAN MEDDL_USE_SHADERC)
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
