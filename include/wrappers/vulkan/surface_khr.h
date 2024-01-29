#pragma once

#include <iostream>
#include <memory>
#include <stdexcept>
#include <string>
#include <utility>

#include "wrappers/glfw/window.h"

namespace meddl::vulkan {

class SurfaceKHR {
 public:
  SurfaceKHR() noexcept = default;
  SurfaceKHR(VkInstance instance, glfw::Window &window) : m_instance(instance) {
    if (glfwCreateWindowSurface(instance, window, nullptr, &m_surface) != VK_SUCCESS) {
      throw std::runtime_error("Surface creation failed");
    }
    std::cout << "Created a surface!\n";
  }

  ~SurfaceKHR() {
    std::cout << "Destroyed a surface!\n";
    vkDestroySurfaceKHR(m_instance, m_surface, nullptr);
  }
  // TODO: Sort these out later..
  SurfaceKHR(SurfaceKHR &&other) noexcept = default;
  // Surfaces are non-copyable
  SurfaceKHR(const SurfaceKHR &) = delete;
  SurfaceKHR &operator=(SurfaceKHR &&other) noexcept {
    if (this != &other) {
      m_surface = std::move(other.m_surface);
      m_instance = std::move(other.m_instance);
    }
    std::cout << "Exiting move assignment..\n";
    return *this;
  };
  // Surfaces are non-copyable
  SurfaceKHR &operator=(const SurfaceKHR &) = delete;

  operator VkSurfaceKHR() const noexcept {
    return m_surface;
  }

  bool is_surface_nulltr() {
    return m_surface == nullptr;
  }

 private:
  VkSurfaceKHR m_surface{nullptr};
  VkInstance m_instance{nullptr};
};

}  // namespace meddl::vulkan
