include(FetchContent)

FetchContent_Declare(
  spirv-tools
  GIT_REPOSITORY https://github.com/KhronosGroup/SPIRV-Tools
  GIT_TAG 44d72a9b36702f093dd20815561a56778b2d181e
)

FetchContent_MakeAvailable(spirv-tools)
