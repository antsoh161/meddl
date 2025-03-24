#pragma once

#include <format>
#include <source_location>
#include <string>
#include <string_view>

namespace meddl::error {

class Error {
  public:
   Error(std::string_view message, std::source_location loc = std::source_location::current())
       : _message(message), _location(loc)
   {
   }

   [[nodiscard]] const std::string& message() const { return _message; }

   [[nodiscard]] const std::source_location& location() const { return _location; }

   template <typename... Args>
   static Error format(std::string_view fmt,
                       Args&&... args,
                       std::source_location loc = std::source_location::current())
   {
      return Error{std::format(fmt, std::forward<Args>(args)...), loc};
   }

   [[nodiscard]] std::string full_message() const
   {
      return std::format("Error at {}:{}: {}", _location.file_name(), _location.line(), _message);
   }

  private:
   std::string _message;
   std::source_location _location;
};

}  // namespace meddl::error
