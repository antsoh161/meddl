
#include "core/engine.h"

#include <vulkan/vulkan_core.h>

#include <iostream>
#include <optional>
#include <span>
#include <stdexcept>

#include "GLFW/glfw3.h"
#include "vulkan_renderer/vulkan_debug.h"

const std::vector<const char *> Engine::get_required_extensions() {
  uint32_t glfw_ext_count = 0;
  auto glfw_ext = std::span(glfwGetRequiredInstanceExtensions(&glfw_ext_count), glfw_ext_count);
  std::vector<const char *> extensions(glfw_ext.begin(), glfw_ext.end());
#ifndef NDEBUG
  extensions.emplace_back(VK_EXT_DEBUG_UTILS_EXTENSION_NAME);
#endif
  return extensions;
}

std::vector<VkPhysicalDevice> Engine::get_suitable_devices() {
  std::vector<VkPhysicalDevice> suitable{};

  uint32_t n_devices = 0;
  vkEnumeratePhysicalDevices(m_instance, &n_devices, nullptr);
  if (n_devices < 1) throw std::runtime_error("No Vulkan compatible GPUs found");

  std::vector<VkPhysicalDevice> devices(n_devices);
  vkEnumeratePhysicalDevices(m_instance, &n_devices, devices.data());

  // TODO: Which indices to pick?
  for (const auto device : devices) {
    if (QueueFamilyIndices(device).m_graphics.has_value()) {
      suitable.push_back(device);
    }
  }
  return suitable;
}

void Engine::create_logical_device() {
  auto indicies = QueueFamilyIndices(m_active_physical_device);

  VkDeviceQueueCreateInfo queue_cinfo{};
  queue_cinfo.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
  queue_cinfo.queueFamilyIndex = indicies.m_graphics.value();
  queue_cinfo.queueCount = 1;
  queue_cinfo.queueCount = 1.0f;

  VkPhysicalDeviceFeatures features{};
  VkDeviceCreateInfo create_info;
  create_info.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
  create_info.pQueueCreateInfos = &queue_cinfo;
  create_info.queueCreateInfoCount = 1;
  create_info.pEnabledFeatures = &features;
  create_info.enabledExtensionCount = 0;

// TODO: Validation layers should be decided on config
#ifndef NDEBUG
  create_info.enabledLayerCount = static_cast<uint32_t>(
      m_debugger.get_active_validation_layers().size());
#else

#endif
}

Engine::~Engine() {
  auto func = reinterpret_cast<PFN_vkDestroyDebugUtilsMessengerEXT>(
      vkGetInstanceProcAddr(m_instance, "vkDestroyDebugUtilsMessengerEXT"));
  if (func != nullptr) func(m_instance, *m_debugger.get_messenger(), nullptr);
  glfwTerminate();
}

void Engine::make_instance() {
}

void Engine::init_glfw() {
  glfwInit();
  glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
  glfwWindowHint(GLFW_RESIZABLE, GLFW_FALSE);

  WindowBuilder wb;
  m_windows.push_back(wb.build());
}

void Engine::init_vulkan() {
  // TODO: This should not be hard coded
#ifndef NDEBUG
  m_debugger.add_validation_layer("VK_LAYER_KHRONOS_validation");
#endif

  VkApplicationInfo app_info{};
  app_info.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
  app_info.pApplicationName = "Meddl Engine";
  app_info.applicationVersion = VK_MAKE_VERSION(0, 1, 0);
  app_info.pEngineName = "Internal";
  app_info.engineVersion = VK_MAKE_VERSION(0, 1, 0);
  app_info.apiVersion = VK_API_VERSION_1_0;

  VkInstanceCreateInfo create_info{};
  create_info.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
  create_info.pApplicationInfo = &app_info;

  auto extensions = get_required_extensions();
  create_info.enabledExtensionCount = extensions.size();
  create_info.ppEnabledExtensionNames = extensions.data();

  // Debug
  VkDebugUtilsMessengerCreateInfoEXT debug_create_info = default_info();
#ifndef NDEBUG
  create_info.enabledLayerCount = m_debugger.get_active_validation_layers().size();
  // TODO: Refactor if this is needed elsewhere
  std::vector<const char *> layers_cstyle(m_debugger.get_active_validation_layers().size());
  for (const auto &str : m_debugger.get_active_validation_layers())
    layers_cstyle.push_back(str.c_str());
  create_info.ppEnabledLayerNames = layers_cstyle.data();

  create_info.pNext = (VkDebugUtilsMessengerCreateInfoEXT *)&debug_create_info;
#else
  create_info.enabledLayerCount = 0;
  create_info.pNext = nullptr;
#endif
  if (vkCreateInstance(&create_info, nullptr, &m_instance) != VK_SUCCESS)
    throw std::runtime_error("Vulkan instance could not be created");

    // Setup debug messenger
#ifndef NDEBUG
  auto func = reinterpret_cast<PFN_vkCreateDebugUtilsMessengerEXT>(
      vkGetInstanceProcAddr(m_instance, "vkCreateDebugUtilsMessengerEXT"));
  if (func(m_instance, &debug_create_info, nullptr, m_debugger.get_messenger()) != VK_SUCCESS)
    throw std::runtime_error("Debug messenger creation failed");
#endif

  auto suitables = get_suitable_devices();
  // TODO: Allow user to pick and update GPU
  if (suitables.size() > 0) {
    m_active_physical_device = suitables.front();
  } else {
    throw std::runtime_error("No suitable GPU found");
  }
}

void Engine::run() {
  while (true) {
    glfwPollEvents();
  }
}
