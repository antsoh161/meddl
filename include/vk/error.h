#pragma once

#include <cstdint>

namespace meddl::vk {
enum class CommandError : uint8_t {
   NotReady,
   NotRecording,
   NotExecutable,
};
}
enum class ShaderError : uint8_t {
   CompilationFailed,
   InvalidPath,
};  // namespace meddl::error
