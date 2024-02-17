cmake_minimum_required(VERSION 3.25.1)
project(MeddlExamples)

add_executable(MeddlExample1 main.cpp)
target_include_directories(MeddlExample1 PRIVATE ${Meddl_SOURCE_DIR}/include)
target_link_libraries(MeddlExample1 PUBLIC Meddl pthread)
target_compile_options(
  MeddlExample1
  PRIVATE $<$<CONFIG:Release>:-O2>
  PRIVATE $<$<CONFIG:Debug>:-O0>)

if(CMAKE_CXX_COMPILER_ID MATCHES "Clang" OR CMAKE_CXX_COMPILER_ID MATCHES "GNU")
   target_compile_options(MeddlExample1 PRIVATE -Wall -Wextra -pedantic )
endif()
