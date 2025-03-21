project(tinygltf)

include(FetchContent)

FetchContent_Declare(
    tinygltf
    GIT_REPOSITORY https://github.com/syoyo/tinygltf
    GIT_TAG v2.9.5
    GIT_SHALLOW TRUE
)
FetchContent_MakeAvailable(tinygltf)

# shush
if(CMAKE_CXX_COMPILER_ID MATCHES "Clang" OR CMAKE_CXX_COMPILER_ID MATCHES "GNU")
    target_compile_options(tinygltf INTERFACE
        "-Wno-unused-parameter"
        "-Wno-unused-variable"
        "-Wno-deprecated-declarations"
        "-Wno-missing-field-initializers")
elseif(MSVC)
    target_compile_options(tinygltf INTERFACE
        /wd4100  # unreferenced formal parameter
        /wd4189  # local variable is initialized but not referenced
        /wd4244  # conversion possible loss of data
        /wd4267  # conversion from 'size_t' to 'type'
    )
endif()
