cmake_minimum_required(VERSION 3.25.1)
project(MeddlExamples)

add_executable(MeddlBall bouncing_ball/ball.cpp)
target_include_directories(MeddlBall PRIVATE ${Meddl_SOURCE_DIR}/include bouncing_ball)
target_link_libraries(MeddlBall PUBLIC Meddl
    $<$<NOT:$<PLATFORM_ID:Windows>>:pthread>
)
target_compile_options(
  MeddlBall
  PRIVATE $<$<CONFIG:Release>:-O2>
  # PRIVATE $<$<CONFIG:Debug>:-fno-omit-frame-pointer -fsanitize=address>
  PRIVATE $<$<CONFIG:Debug>:-O0>)

# target_link_options(
#   MeddlBall
#   PRIVATE $<$<CONFIG:Debug>:-fsanitize=address>)

if(CMAKE_CXX_COMPILER_ID MATCHES "Clang" OR CMAKE_CXX_COMPILER_ID MATCHES "GNU")
   target_compile_options(MeddlBall PRIVATE -Wall -Wextra -pedantic )
endif()

add_executable(MeddlSpaceship spaceship/spaceship.cpp)
target_include_directories(MeddlSpaceship PRIVATE ${Meddl_SOURCE_DIR}/include spaceship)
target_link_libraries(MeddlSpaceship PUBLIC Meddl
    $<$<NOT:$<PLATFORM_ID:Windows>>:pthread>
)
target_compile_options(
  MeddlSpaceship
  PRIVATE $<$<CONFIG:Release>:-O2>
  # PRIVATE $<$<CONFIG:Debug>:-fno-omit-frame-pointer -fsanitize=address>
  PRIVATE $<$<CONFIG:Debug>:-O0>)

# target_link_options(
#   MeddlBall
#   PRIVATE $<$<CONFIG:Debug>:-fsanitize=address>)

if(CMAKE_CXX_COMPILER_ID MATCHES "Clang" OR CMAKE_CXX_COMPILER_ID MATCHES "GNU")
   target_compile_options(MeddlSpaceship PRIVATE -Wall -Wextra -pedantic )
endif()
