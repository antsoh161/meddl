#pragma once

#include <format>
#include <source_location>

namespace meddl::error {
class Error {
  public:
   constexpr Error(std::string_view message,
                   std::source_location location = std::source_location::current()) noexcept
       : _message(message), _location(location)
   {
   }

   [[nodiscard]] constexpr std::string_view message() const noexcept { return _message; }
   [[nodiscard]] constexpr const std::source_location& location() const noexcept
   {
      return _location;
   }

   [[nodiscard]] std::string format() const
   {
      return std::format("{}({}): {}", _location.file_name(), _location.line(), _message);
   }

  private:
   std::string_view _message;
   std::source_location _location;
};

template <typename... Args>
struct FormatString {
   template <size_t N>
   constexpr FormatString(const char (&fmt)[N]) : view(fmt)
   {
   }
   std::string_view view;
};

namespace detail {
template <typename... Args>
struct FormattedError {
   FormatString<Args...> fmt;
   std::tuple<Args...> args;

   [[nodiscard]] std::string message() const
   {
      return std::apply([this](const auto&... args) { return std::format(fmt.view, args...); },
                        args);
   }
};
}  // namespace detail

template <typename... Args>
auto make_formatted_error(FormatString<Args...> fmt, Args&&... args)
{
   return detail::FormattedError<Args...>{fmt, std::make_tuple(std::forward<Args>(args)...)};
}


}  // namespace meddl::error
