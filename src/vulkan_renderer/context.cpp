#include "vulkan_renderer/context.h"

#include <vulkan/vulkan_core.h>

#include <memory>
#include <span>
#include <utility>

#include "core/asserts.h"
#include "vulkan_renderer/device.h"

namespace meddl::vk {

Context::Context(std::shared_ptr<glfw::Window>&& window,
                 std::optional<VulkanDebugger>&& debugger,
                 VkDebugUtilsMessengerCreateInfoEXT debug_info,
                 VkApplicationInfo app_info,
                 const std::set<PhysicalDeviceQueueProperties>& pdr,
                 const std::set<std::string>& device_extensions,
                 const SwapChainOptions& swapchain_options)
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
      throw std::runtime_error("No GPU found with the requirements passed to context builder");
   }

   if (!make_logical_device(device_extensions)) {
      clean_up();
      throw std::runtime_error("Logical device creation failed");
   }
   try {
      make_swapchain(swapchain_options);
   }
   catch (const std::exception& e) {
      clean_up();
      throw;
   }
}

Context::~Context()
{
   clean_up();
}

void Context::clean_up()
{
   for(auto& image_view : _swapchain.get_image_views())
   {
      vkDestroyImageView(_logical_device, image_view, nullptr);
   }
   vkDestroySwapchainKHR(_logical_device, _swapchain, nullptr);
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

void Context::make_swapchain(const SwapChainOptions& swapchain_options)
{
   VkSurfaceCapabilitiesKHR surface_capabilities{};
   vkGetPhysicalDeviceSurfaceCapabilitiesKHR(*_active_device, _surface, &surface_capabilities);
   VkExtent2D extent_2D;
   // TODO: 3D?
   if (surface_capabilities.currentExtent.width != std::numeric_limits<uint32_t>::max()) {
      extent_2D = surface_capabilities.currentExtent;
   }
   else {
      const auto fbs = _window->get_framebuffer_size();
      extent_2D = {std::clamp(fbs.width,
                              surface_capabilities.minImageExtent.width,
                              surface_capabilities.maxImageExtent.width),
                   std::clamp(fbs.height,
                              surface_capabilities.minImageExtent.height,
                              surface_capabilities.maxImageExtent.height)};
   }

   uint32_t format_count{};
   vkGetPhysicalDeviceSurfaceFormatsKHR(*_active_device, _surface, &format_count, nullptr);

   std::vector<VkSurfaceFormatKHR> formats;
   if (format_count == 0) {
      throw std::runtime_error("No swapchain formats found");
   }
   formats.resize(format_count);
   vkGetPhysicalDeviceSurfaceFormatsKHR(*_active_device, _surface, &format_count, formats.data());

   std::unordered_set<VkSurfaceFormatKHR> format_set;
   for (const auto& format : formats) {
      format_set.insert(format);
   }

   uint32_t present_modes_count{};
   vkGetPhysicalDeviceSurfacePresentModesKHR(
       *_active_device, _surface, &present_modes_count, nullptr);

   std::vector<VkPresentModeKHR> present_modes{};
   if (present_modes_count == 0) {
      throw std::runtime_error("No swapchain present modes found");
   }
   present_modes.resize(present_modes_count);
   vkGetPhysicalDeviceSurfacePresentModesKHR(
       *_active_device, _surface, &present_modes_count, present_modes.data());

   std::unordered_set<VkPresentModeKHR> present_mode_set{};
   for (const auto& present_mode : present_modes) {
      present_mode_set.insert(present_mode);
   }

   uint32_t min_image_count = swapchain_options.image_count.has_value()
                              ? swapchain_options.image_count.value()
                              : surface_capabilities.minImageCount + 1;
   // 0 max means there's no maximum to image count
   if (surface_capabilities.maxImageCount > 0 && min_image_count > surface_capabilities.maxImageCount) {
      min_image_count = surface_capabilities.maxImageCount;
   }

   VkSwapchainCreateInfoKHR swapchain_info{};
   swapchain_info.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
   swapchain_info.surface = _surface;
   swapchain_info.minImageCount = min_image_count;
   auto found_format = format_set.find(swapchain_options.surface_format);
   if (found_format != format_set.end()) {
      swapchain_info.imageFormat = found_format->format;
      swapchain_info.imageColorSpace = found_format->colorSpace;
   }
   else {
      std::cerr << "Requested swapchain format not available, defaulting to first found\n";
      swapchain_info.imageFormat = format_set.cbegin()->format;
      swapchain_info.imageColorSpace = format_set.cbegin()->colorSpace;
   }
   swapchain_info.imageExtent = extent_2D;
   swapchain_info.imageArrayLayers = swapchain_options.image_array_layers;  // TODO: what is this?
   swapchain_info.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;  // TODO: swapchain option?

   auto qf = _active_device->get_queue_families();
   if (qf.size() > 1) {
      M_TODO("Swapchain multi-queue families not implemented");
   }
   swapchain_info.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
   swapchain_info.queueFamilyIndexCount = 0;                             // TODO: optional?
   swapchain_info.pQueueFamilyIndices = nullptr;                         // TODO: optional?
   swapchain_info.preTransform = surface_capabilities.currentTransform;  // No transformation
   swapchain_info.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;    // TODO: Swapchain option
   auto found_present_mode = present_mode_set.find(swapchain_options.present_mode);

   swapchain_info.presentMode = found_present_mode != present_mode_set.end()
                                    ? *found_present_mode
                                    : *present_mode_set.cbegin();
   swapchain_info.clipped = VK_TRUE;  // TODO: swapchain option
   swapchain_info.oldSwapchain = VK_NULL_HANDLE;
   _swapchain = SwapChain(
       _logical_device, format_set, present_mode_set, surface_capabilities, swapchain_info);
}

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

ContextBuilder& ContextBuilder::with_swapchain_options(const SwapChainOptions& swapchain_options)
{
   _swapchain_options = swapchain_options;
   return *this;
}

Context ContextBuilder::build()
{
   M_ASSERT(_window, "Can NOT create a vulkan context without a valid window");
   if (_debugger.has_value()) {
      _debugger->add_validation_layers(_debug_layers);
   }
   return {std::move(_window),
           std::move(_debugger),
           _debug_info,
           _app_info,
           _pdr,
           _device_extensions,
           _swapchain_options};
}
}  // namespace meddl::vk
