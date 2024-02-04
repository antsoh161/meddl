project(glfw)
set(FETCHCONTENT_QUIET OFF) # Debug
include(FetchContent)

FetchContent_Declare(
  glfw
  GIT_REPOSITORY https://github.com/glfw/glfw.git
  GIT_TAG 7482de6071d21db77a7236155da44c172a7f6c9e # 3.3.8
)
FetchContent_MakeAvailable(glfw)
