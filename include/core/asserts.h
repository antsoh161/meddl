#include <format>
#include <iostream>
#include <stdexcept>

#define M_ASSERT(expr, ...)                                                    \
  if (expr) {                                                                  \
  } else {                                                                     \
    std::cerr << std::format("assertion failed: {} at {}:{}", #expr, __FILE__, \
                             __LINE__, std::format(__VA_ARGS__))               \
              << std::endl;                                                    \
    throw std::runtime_error("assertion failed");                              \
  }

#define M_ASSERT_U(...)                                                  \
  std::cerr << std::format("unconditional assertion failed: at {}:{}",   \
                           __FILE__, __LINE__, std::format(__VA_ARGS__)) \
            << std::endl;                                                \
  throw std::runtime_error("Assertion failed");

#define M_ASSERT_NOTNULL(expr) \
  if (expr != nullptr) {                                                     \
  } else {                                                                   \
    std::cerr << std::format("nullptr assertion failed: {} at {}:{}", #expr, \
                             __FILE__, __LINE__)   \
              << std::endl;                                                  \
    throw std::runtime_error("assertion failed");                            \
  }
