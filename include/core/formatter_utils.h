#pragma once


#include <format>
namespace meddl::utils {

template<typename T>
concept formattable = requires (T& v, std::format_context ctx) {
  std::formatter<std::remove_cvref_t<T>>().format(v, ctx);
};
}  // namespace meddl::utils
