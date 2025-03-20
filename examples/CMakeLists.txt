cmake_minimum_required(VERSION 3.25.1)
project(MeddlExamples)

add_executable(MeddlBall bouncing_ball/ball.cpp)
target_include_directories(MeddlBall PRIVATE ${Meddl_SOURCE_DIR}/include)
target_link_libraries(MeddlBall PUBLIC Meddl pthread)
target_compile_options(
  MeddlBall
  PRIVATE $<$<CONFIG:Release>:-O2>
  PRIVATE $<$<CONFIG:Debug>:-O0>)

if(CMAKE_CXX_COMPILER_ID MATCHES "Clang" OR CMAKE_CXX_COMPILER_ID MATCHES "GNU")
   target_compile_options(MeddlBall PRIVATE -Wall -Wextra -pedantic )
endif()
