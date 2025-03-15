#pragma once

#include <cstdint>
#include <format>
#include <system_error>

namespace meddl::error {

class VulkanErrorCategory : public std::error_category {
  public:
   [[nodiscard]] const char* name() const noexcept override { return "VulkanError"; }

   [[nodiscard]] std::string message(int ev) const override;

   static constexpr VulkanErrorCategory& get() noexcept
   {
      static VulkanErrorCategory instance;
      return instance;
   }
};

enum class VulkanError : uint32_t {
   CommandBufferNotReady,
   CommandBufferNotRecording,
   CommandBufferNotExecutable,
   ShaderCompilationFailed,
   ShaderFileReadError,
   ShaderInvalidCompiler,
};

constexpr std::error_code make_error_code(VulkanError e) noexcept
{
   return {static_cast<int>(e), VulkanErrorCategory::get()};
}

struct ErrorInfo {
   std::error_code code;
   std::string details;

   [[nodiscard]] constexpr inline std::string message() const
   {
      if (details.empty()) {
         return code.message();
      }
      return std::format("{}:{}: {}", code.value(), code.message(), details);
   }
};

template <typename... Args>
ErrorInfo make_error(std::error_code code, std::format_string<Args...> fmt, Args&&... args)
{
   return {code, std::format(fmt, std::forward<Args>(args)...)};
}

template <typename... Args>
ErrorInfo make_error(VulkanError code, std::format_string<Args...> fmt, Args&&... args)
{
   return {code, std::format(fmt, std::forward<Args>(args)...)};
}
}  // namespace meddl::error

namespace std {
template <>
struct is_error_code_enum<meddl::error::VulkanError> : true_type {};
}  // namespace std
