project(stdexec)
set(FETCHCONTENT_QUIET OFF) # Debug
include(FetchContent)

FetchContent_Declare(
 stdexec
  GIT_REPOSITORY https://github.com/NVIDIA/stdexec.git
  GIT_TAG 9514e7bdf4b5d16d8ee4b5ad0e9c8733c3539f37
)

FetchContent_MakeAvailable(stdexec)
