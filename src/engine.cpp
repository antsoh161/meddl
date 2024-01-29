#include "core/engine.h"

#include <vulkan/vulkan_core.h>

#include <iostream>
#include <optional>
#include <span>
#include <stdexcept>

#include "vulkan_renderer/device.h"
#include "vulkan_renderer/vulkan_debug.h"
#include "wrappers/glfw/glfw_wrapper.h"
#include "wrappers/glfw/window.h"

const std::vector<const char *> Engine::get_required_extensions() {
   uint32_t glfw_ext_count = 0;
   auto glfw_ext = glfwGetRequiredInstanceExtensions(&glfw_ext_count);
   auto ext_span = std::span(glfw_ext, glfw_ext_count);
   std::vector<const char *> extensions(ext_span.begin(), ext_span.end());

#ifndef NDEBUG
   extensions.push_back(VK_EXT_DEBUG_UTILS_EXTENSION_NAME);
#endif
   return extensions;
}

void Engine::create_logical_device() {
   float queue_prio = 1.0f;
   auto make_cinfo = [&queue_prio](unsigned int index) -> VkDeviceQueueCreateInfo {
      VkDeviceQueueCreateInfo queue_cinfo{};
      queue_cinfo.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
      queue_cinfo.queueFamilyIndex = index;
      queue_cinfo.queueCount = 1;
      queue_cinfo.queueCount = 1.0f;
      queue_cinfo.pQueuePriorities = &queue_prio;
      return queue_cinfo;
   };

   std::vector<VkDeviceQueueCreateInfo> queue_create_infos;
   auto graphics_index = m_active_physical_device->get_graphics_index();
   auto present_index = m_active_physical_device->get_present_index();

   if (graphics_index.has_value()) {
      queue_create_infos.push_back(make_cinfo(graphics_index.value()));
   }
   if (present_index.has_value()) {
      queue_create_infos.push_back(make_cinfo(present_index.value()));
   }

   VkPhysicalDeviceFeatures features{};
   VkDeviceCreateInfo create_info;
   create_info.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
   create_info.queueCreateInfoCount = queue_create_infos.size();
   create_info.pQueueCreateInfos = queue_create_infos.data();
   create_info.pEnabledFeatures = &features;
   create_info.enabledExtensionCount = 0;

// // TODO: Validation layers should be decided on config
#ifndef NDEBUG
   create_info.enabledLayerCount =
       static_cast<uint32_t>(m_debugger.get_active_validation_layers().size());
   // TODO: can be refactored...
   auto layers = m_debugger.get_active_validation_layers();
   std::vector<const char *> layers_cstyle(m_debugger.get_active_validation_layers().size());
   std::transform(layers.begin(), layers.end(), layers_cstyle.begin(), [](const std::string &s) {
      return s.c_str();
   });

   create_info.ppEnabledLayerNames = layers_cstyle.data();
#else
   create_info.enabledLayerCount = 0;
#endif
   //
   if (vkCreateDevice(*m_active_physical_device, &create_info, nullptr, &m_active_logical_device) !=
       VK_SUCCESS)
      throw std::runtime_error("Failed to create logical device!");
   if (graphics_index.has_value()) {
      vkGetDeviceQueue(m_active_logical_device, graphics_index.value(), 0, &m_graphics_queue);
   }
   if (present_index.has_value()) {
      vkGetDeviceQueue(m_active_logical_device, present_index.value(), 0, &m_present_queue);
   }
}

Engine::~Engine() {
   vkDestroyDevice(m_active_logical_device, nullptr);

   auto func = reinterpret_cast<PFN_vkDestroyDebugUtilsMessengerEXT>(
       vkGetInstanceProcAddr(m_instance, "vkDestroyDebugUtilsMessengerEXT"));
   if (func != nullptr) func(m_instance, *m_debugger.get_messenger(), nullptr);

   vkDestroyInstance(m_instance, nullptr);
   glfwTerminate();
}

void Engine::make_instance() {
}

void Engine::init_glfw() {
   glfwInit();
   glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
   glfwWindowHint(GLFW_RESIZABLE, GLFW_FALSE);

   // TODO: Builder configuration? Fullscreen etc.
   // WindowBuilder wb;
   // auto w = wb.build();
   // m_windows.push_back(wb.build());
   // m_windows.emplace_back(800, 600, "Meddl Engine");
   m_windows.emplace_back(800, 600, "Meddl");
}

void Engine::create_physical_devices() {
   uint32_t n_devices = 0;
   vkEnumeratePhysicalDevices(m_instance, &n_devices, nullptr);
   if (n_devices < 1) throw std::runtime_error("No Vulkan compatible GPUs found");

   std::vector<VkPhysicalDevice> devices(n_devices);
   vkEnumeratePhysicalDevices(m_instance, &n_devices, devices.data());

   for (const auto &device : devices) {
      m_physical_devices.emplace_back(device, m_surface);
   }
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
   auto layers = m_debugger.get_active_validation_layers();
   std::vector<const char *> layers_cstyle(m_debugger.get_active_validation_layers().size());
   std::transform(layers.begin(), layers.end(), layers_cstyle.begin(), [](const std::string &s) {
      return s.c_str();
   });
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

   m_surface = meddl::vulkan::SurfaceKHR(m_instance, m_windows[0]);
   // if(m_surface.is_surface_nulltr())
   //   std::cout << "The surface is null..\n";
   // if(glfwCreateWindowSurface(m_instance, m_windows[0], nullptr, &m_bajs_surface) != VK_SUCCESS)
   // {
   //   throw std::runtime_error("Surface creation failed");
   // }

   create_physical_devices();

   // We pick the first one for now, make decidable later i guess
   for (const auto &device : m_physical_devices) {
      if (device.get_graphics_index().has_value() && device.get_present_index().has_value()) {
         m_active_physical_device = std::make_unique<PhysicalDevice>(device);
         break;
      }
   }
   if (!m_active_physical_device) {
      throw std::runtime_error("No suitable GPU found");
   }
   // Logical Devices
   // create_logical_device();
}

void Engine::run() {
   while (true) {
      glfwPollEvents();
   }
}
