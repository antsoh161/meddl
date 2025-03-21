# include(FetchContent)
#
# FetchContent_Declare(
#   SPIRV-Headers
#   GIT_REPOSITORY https://github.com/KhronosGroup/SPIRV-Headers
#   GIT_TAG 1c6bb2743599e6eb6f37b2969acc0aef812e32e3 #vulkan-sdk-1.3.275.0
# )
# FetchContent_MakeAvailable(SPIRV-Headers)
#
# FetchContent_Declare(
#   SPIRV-Tools
#   GIT_REPOSITORY https://github.com/KhronosGroup/SPIRV-Tools
#   GIT_TAG f0cc85efdbbe3a46eae90e0f915dc1509836d0fc #vulkan-sdk-1.3.275.0
# )
# FetchContent_MakeAvailable(SPIRV-Tools)
#
# FetchContent_Declare(
#   glslang
#   GIT_REPOSITORY https://github.com/KhronosGroup/glslang
#   GIT_TAG a91631b260cba3f22858d6c6827511e636c2458a # 14.0.0
# )
# FetchContent_MakeAvailable(glslang)
#

FetchContent_Declare(
  shaderc
  GIT_REPOSITORY https://github.com/google/shaderc.git
  GIT_TAG e6edd6d48fa5bdd9d176794c6810fae7f8e938e1 #v2023.8
)
FetchContent_MakeAvailable(shaderc)
