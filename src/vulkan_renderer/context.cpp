#include "vulkan_renderer/context.h"

#include <vulkan/vulkan_core.h>

#include <memory>
#include <span>
#include <utility>

#include "core/asserts.h"
#include "vulkan_renderer/device.h"

namespace meddl::vulkan {

Context::Context(std::shared_ptr<glfw::Window>&& window,
                 std::optional<VulkanDebugger>&& debugger,
                 VkDebugUtilsMessengerCreateInfoEXT debug_info,
                 VkApplicationInfo app_info,
                 const std::set<PhysicalDeviceQueueProperties>& pdr,
                 const std::set<std::string>& device_extensions)
    : _window(std::move(window)), _debugger(std::move(debugger))
{
   if (make_instance(app_info, debug_info) != VK_SUCCESS) {
      clean_up();
      throw std::runtime_error("Vulkan instance creation failed");
   }

   if (_debugger.has_value()) {
      auto func = reinterpret_cast<PFN_vkCreateDebugUtilsMessengerEXT>(         // NOLINT
          vkGetInstanceProcAddr(_instance, "vkCreateDebugUtilsMessengerEXT"));  // NOLINT
      // TODO: raw pointer?
      if (func(_instance, &debug_info, nullptr, _debugger->get_messenger()) != VK_SUCCESS) {
         clean_up();
         throw std::runtime_error("Debug messenger creation failed");
      }
   }

   if (glfwCreateWindowSurface(_instance, *_window, nullptr, &_surface) != VK_SUCCESS) {
      clean_up();
      throw std::runtime_error("Surface creation failed");
   }

   if (!make_physical_devices()) {
      clean_up();
      throw std::runtime_error("No vulkan compatible GPUs found");
   }

   if (!pick_suitable_device(pdr, device_extensions)) {
      clean_up();
      throw std::runtime_error("No GPU has the requirements passed to context builder");
   }

   if (!make_logical_device(device_extensions)) {
      clean_up();
      throw std::runtime_error("Logical device creation failed");
   }
   // if(!make_swapchain())
   // {
   //    clean_up();
   //    throw std::runtime_error("Swapchain creation failed");
   // }
   _swapchain.populate_swapchain(*_active_device, _surface);
}

Context::~Context()
{
   clean_up();
}

void Context::clean_up()
{
   vkDestroyDevice(_logical_device, nullptr);
   if (_debugger.has_value()) {
      _debugger->clean_up(_instance);
   }
   vkDestroySurfaceKHR(_instance, _surface, nullptr);
   vkDestroyInstance(_instance, nullptr);
}

VkResult Context::make_instance(const VkApplicationInfo& app_info,
                                const VkDebugUtilsMessengerCreateInfoEXT& debug_info)
{
   VkInstanceCreateInfo instance_create_info{};
   instance_create_info.sType =
       VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;  // TODO: can there be others?
   instance_create_info.pApplicationInfo = &app_info;

   uint32_t glfw_ext_count = 0;
   auto glfw_ext = glfwGetRequiredInstanceExtensions(&glfw_ext_count);
   auto ext_span = std::span(glfw_ext, glfw_ext_count);
   std::vector<const char*> extensions(ext_span.begin(), ext_span.end());

   std::vector<const char*> layers_cstyle{};
   if (_debugger.has_value()) {
      extensions.push_back(VK_EXT_DEBUG_UTILS_EXTENSION_NAME);
      instance_create_info.enabledLayerCount = _debugger->get_active_validation_layers().size();

      const auto& layers = _debugger->get_active_validation_layers();
      for (const auto& layer : layers) {
         layers_cstyle.push_back(layer.c_str());
      }
      instance_create_info.ppEnabledLayerNames = layers_cstyle.data();
      instance_create_info.pNext = &debug_info;
   }
   else {
      instance_create_info.enabledLayerCount = 0;
      instance_create_info.pNext = nullptr;
   }
   instance_create_info.enabledExtensionCount = extensions.size();
   instance_create_info.ppEnabledExtensionNames = extensions.data();

   return vkCreateInstance(&instance_create_info, nullptr, &_instance);
}

bool Context::make_physical_devices()
{
   uint32_t n_devices = 0;
   vkEnumeratePhysicalDevices(_instance, &n_devices, nullptr);
   if (n_devices < 1) {
      return false;
   }

   std::vector<VkPhysicalDevice> devices(n_devices);
   vkEnumeratePhysicalDevices(_instance, &n_devices, devices.data());

   for (const auto& device : devices) {
      _available_devices.emplace_back(device, _surface);
   }
   return true;
}

bool Context::pick_suitable_device(const std::set<PhysicalDeviceQueueProperties>& requested_pdr,
                                   const std::set<std::string>& requested_extensions)
{
   std::vector<PhysicalDevice*> candidates;
   for (auto& device : _available_devices) {
      if (device.fulfills_requirement(requested_pdr, requested_extensions)) {
         candidates.push_back(&device);
      }
   }
   if (candidates.size() > 0) {
      _active_device = candidates[0];
      return true;
   }
   std::cout << "NO SUITABLE FOUND\n";
   return false;
}

bool Context::make_logical_device(const std::set<std::string>& device_extensions)
{
   // TODO: This might also be an initialization option
   float queue_prio = 1.0f;
   auto make_cinfo = [&queue_prio](uint32_t index) -> VkDeviceQueueCreateInfo {
      VkDeviceQueueCreateInfo queue_cinfo{};
      queue_cinfo.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
      queue_cinfo.queueFamilyIndex = index;
      queue_cinfo.queueCount = 1;  // TODO: 1?
      queue_cinfo.pQueuePriorities = &queue_prio;
      return queue_cinfo;
   };

   std::vector<VkDeviceQueueCreateInfo> queue_create_infos;
   for (const auto& family : _active_device->get_queue_families()) {
      queue_create_infos.push_back(make_cinfo(family._idx));
   }

   VkPhysicalDeviceFeatures features{};
   VkDeviceCreateInfo create_info{};
   create_info.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
   create_info.queueCreateInfoCount = queue_create_infos.size();
   create_info.pQueueCreateInfos = queue_create_infos.data();
   create_info.pEnabledFeatures = &features;
   create_info.enabledExtensionCount = static_cast<uint32_t>(device_extensions.size());

   std::vector<const char*> extensions_cstyle{};
   for (const auto& ext : device_extensions) {
      extensions_cstyle.push_back(ext.c_str());
   }
   create_info.ppEnabledExtensionNames = extensions_cstyle.data();

   std::vector<const char*> layers_cstyle{};
   if (_debugger.has_value()) {
      create_info.enabledLayerCount = _debugger->get_active_validation_layers().size();
      const auto& layers = _debugger->get_active_validation_layers();
      for (const auto& layer : layers) {
         layers_cstyle.push_back(layer.c_str());
      }
      create_info.ppEnabledLayerNames = layers_cstyle.data();
   }
   else {
      create_info.enabledLayerCount = 0;
   }

   if (vkCreateDevice(*_active_device, &create_info, nullptr, _logical_device) != VK_SUCCESS) {
      return false;
   }
   for (auto& family : _active_device->get_queue_families()) {
      vkGetDeviceQueue(_logical_device, family._idx, 0, &family._queue);
   }
   return true;
}

// TODO: Delete this function if it's not needed..
// bool Context::make_swapchain() {
//    _swapchain.populate_swapchain(*_active_device, _surface);
//    return true;
// }

ContextBuilder& ContextBuilder::window(std::shared_ptr<glfw::Window> window)
{
   _window = window;
   return *this;
}

ContextBuilder& ContextBuilder::enable_debugger()
{
   _debugger = VulkanDebugger();
   return *this;
}

ContextBuilder& ContextBuilder::with_debug_layers(const std::vector<std::string>& layers)
{
   _debug_layers = layers;
   return *this;
}

ContextBuilder& ContextBuilder::with_debug_create_info(
    const VkDebugUtilsMessengerCreateInfoEXT& debug_info)
{
   _debug_info = debug_info;
   return *this;
}

ContextBuilder& ContextBuilder::with_app_info(const VkApplicationInfo& app_info)
{
   _app_info = app_info;
   return *this;
}

ContextBuilder& ContextBuilder::with_physical_device_requirements(
    const std::set<PhysicalDeviceQueueProperties>& pdr)
{
   _pdr = pdr;
   return *this;
}

ContextBuilder& ContextBuilder::with_physical_device_requirements(
    const PhysicalDeviceQueueProperties& pdr)
{
   _pdr.insert(pdr);
   return *this;
}
ContextBuilder& ContextBuilder::with_required_device_extensions(
    const std::set<std::string>& device_extensions)
{
   _device_extensions = device_extensions;
   return *this;
}

Context ContextBuilder::build()
{
   M_ASSERT(_window, "Can NOT create a vulkan context without a valid window");
   if (_debugger.has_value()) {
      _debugger->add_validation_layers(_debug_layers);
   }
   return {
       std::move(_window), std::move(_debugger), _debug_info, _app_info, _pdr, _device_extensions};
}
}  // namespace meddl::vulkan
