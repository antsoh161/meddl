#pragma once
#include <cstddef>
#include <functional>
#include <numeric>

#include "GLFW/glfw3.h"

namespace {

template <typename T>
T xorshift(const T& n, int i)
{
   return n ^ (n >> i);
}

// a hash function with another name as to not confuse with std::hash
// uint32_t distribute(const uint32_t& n)
// {
//    uint32_t p = 0x55555555ul;                     // NOLINT
//    uint32_t c = 3423571495ul;                     // NOLINT
//    return c * xorshift(p * xorshift(n, 16), 16);  // NOLINT
// }

// a hash function with another name as to not confuse with std::hash
uint64_t distribute(const uint64_t& n)
{
   uint64_t p = 0x5555555555555555ull;            // NOLINT
   uint64_t c = 17316035218449499591ull;          // NOLINT
   return c * xorshift(p * xorshift(n, 32), 32);  // NOLINT
}

// if c++20 rotl is not available:
template <typename T, typename S>
T constexpr rotl(const T n, const S i)
   requires(std::is_unsigned_v<T>)
{
   const T m = (std::numeric_limits<T>::digits - 1);
   const T c = i & m;
   return (n << c) | (n >> ((T(0) - c) & m));  // this is usually recognized by the compiler to mean
                                               // rotation, also c++20 now gives us rotl directly
}

// call this function with the old seed and the new key to be hashed and combined into the new seed
// value, respectively the final hash
template <class T>
inline size_t hash_combine(std::size_t& seed, const T& v)
{
   return rotl(seed, std::numeric_limits<size_t>::digits / 3) ^ distribute(std::hash<T>{}(v));
}
}  // namespace

// Hash functions for vulkan structs
namespace std {

// VkSurfaceFormatKHR
template <>
struct hash<VkSurfaceFormatKHR> {
   std::size_t operator()(const VkSurfaceFormatKHR& surface_format) const noexcept
   {
      std::size_t hash1 = hash<VkFormat>{}(surface_format.format);
      std::size_t hash2 = hash<VkColorSpaceKHR>{}(surface_format.colorSpace);

      return hash_combine(hash1, hash2);
   }
};

template <>
struct hash<VkExtent2D> {
   std::size_t operator()(const VkExtent2D& extent_2d) const noexcept
   {
      std::size_t hash1 = hash<uint32_t>{}(extent_2d.width);
      std::size_t hash2 = hash<uint32_t>{}(extent_2d.height);

      return hash_combine(hash1, hash2);
   }
};
}  // namespace std

// Vulkan comparison operators
constexpr inline bool operator==(const VkSurfaceFormatKHR& lhs, const VkSurfaceFormatKHR& rhs)
{
   return (lhs.format == rhs.format) && (lhs.colorSpace == rhs.colorSpace);
}
