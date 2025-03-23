include(FetchContent)

if(WIN32)
    find_package(shaderc QUIET)
endif()

if(NOT shaderc_FOUND)
    FetchContent_Declare(
        SPIRV-Headers
        GIT_REPOSITORY https://github.com/KhronosGroup/SPIRV-Headers
        GIT_TAG vulkan-sdk-1.4.309.0
    )
    FetchContent_MakeAvailable(SPIRV-Headers)

    set(SPIRV_SKIP_TESTS ON)
    set(SPIRV_SKIP_EXECUTABLES ON)
    FetchContent_Declare(
        SPIRV-Tools
        GIT_REPOSITORY https://github.com/KhronosGroup/SPIRV-Tools
        GIT_TAG vulkan-sdk-1.4.309.0
    )
    FetchContent_MakeAvailable(SPIRV-Tools)

    set(BUILD_TESTING OFF)
    set(ENABLE_GLSLANG_BINARIES OFF)
    FetchContent_Declare(
        glslang
        GIT_REPOSITORY https://github.com/KhronosGroup/glslang
        GIT_TAG vulkan-sdk-1.4.309.0
    )
    FetchContent_MakeAvailable(glslang)

    FetchContent_Declare(
        shaderc
        GIT_REPOSITORY https://github.com/google/shaderc.git
        GIT_TAG v2025.1
    )

    set(SHADERC_SKIP_TESTS ON)
    set(SHADERC_SKIP_EXAMPLES ON)
    set(SHADERC_SKIP_COPYRIGHT_CHECK ON)
    set(SHADERC_SKIP_INSTALL ON)

    FetchContent_MakeAvailable(shaderc)
endif()

if(shaderc_FOUND)
    set(SHADERC_LIB shaderc::shaderc)
else()
    set(SHADERC_LIB shaderc_shared)
endif()
