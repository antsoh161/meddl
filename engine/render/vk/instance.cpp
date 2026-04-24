#include "engine/render/vk/instance.h"

#include <vulkan/vulkan_core.h>

#include <print>

#include "engine/render/vk/debug.h"
#include "engine/render/vk/surface.h"
#include "engine/render/vk/utils.h"

namespace meddl::render::vk {

Instance::~Instance()
{
   if (_debugger && _instance) {
      _debugger->deinit(_instance);
   }
   _physical_devices.clear();
   if (_instance) {
      vkDestroyInstance(_instance, nullptr);
   }
}

Instance::Instance(Instance&& other) noexcept
    : _instance(std::exchange(other._instance, nullptr)),
      _debugger(std::move(other._debugger)),
      _physical_devices(std::move(other._physical_devices))
{
}

Instance& Instance::operator=(Instance&& other) noexcept
{
   if (this != &other) {
      _instance = std::exchange(other._instance, nullptr);
      _physical_devices = std::move(other._physical_devices);
      _debugger = std::move(other._debugger);
   }
   return *this;
}

std::expected<Instance, meddl::error::Error> Instance::create(
    const InstanceConfiguration& config, const std::optional<DebugConfiguration>& debug_config)
{
   Instance instance;

   VkApplicationInfo app_info{
       .sType = VK_STRUCTURE_TYPE_APPLICATION_INFO,
       .pNext = nullptr,
       .pApplicationName = config.app_name.c_str(),
       .applicationVersion = config.app_version,
       .pEngineName = config.engine_name.c_str(),
       .engineVersion = config.engine_version,
       .apiVersion = config.api_version,
   };

   std::vector<const char*> extensions;
   for (const auto& ext : config.extensions) {
      extensions.push_back(ext.c_str());
   }

   std::vector<const char*> layers_cstyle;
   DebugCreateInfoChain debug_chain;
   void* pNext = nullptr;

   if (debug_config.has_value()) {
      instance._debugger = std::make_optional<Debugger>(*debug_config);
      debug_chain = instance._debugger->make_create_info_chain();
      extensions.push_back(VK_EXT_DEBUG_UTILS_EXTENSION_NAME);

      const auto& layers = instance._debugger->get_active_validation_layers();
      for (const auto& layer : layers) {
         layers_cstyle.push_back(layer.c_str());
      }
      pNext = &debug_chain.debug_create_info;
   }

   VkInstanceCreateInfo create_info{
       .sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO,
       .pNext = pNext,
       .pApplicationInfo = &app_info,
       .enabledLayerCount = static_cast<uint32_t>(layers_cstyle.size()),
       .ppEnabledLayerNames = layers_cstyle.empty() ? nullptr : layers_cstyle.data(),
       .enabledExtensionCount = static_cast<uint32_t>(extensions.size()),
       .ppEnabledExtensionNames = extensions.data()};

   auto res = vkCreateInstance(&create_info, nullptr, &instance._instance);
   if (res != VK_SUCCESS) {
      return std::unexpected(
          meddl::error::Error(std::format("Instance creation failed: {}", result_to_string(res))));
   }

   uint32_t n_devices = 0;
   vkEnumeratePhysicalDevices(instance._instance, &n_devices, nullptr);
   if (n_devices < 1) {
      return std::unexpected(meddl::error::Error("No physical devices aquired from instance"));
   }

   std::vector<VkPhysicalDevice> devices(n_devices);
   vkEnumeratePhysicalDevices(instance._instance, &n_devices, devices.data());

   for (const auto& device : devices) {
      instance._physical_devices.emplace_back(&instance, device);
   }
   meddl::log::debug("Created {} physical devices", instance._physical_devices.size());

   if (instance._debugger && instance._instance) {
      instance._debugger->init(instance._instance, debug_chain);
   }
   return instance;
}

std::vector<PhysicalDevice>& Instance::get_physical_devices()
{
   return _physical_devices;
}

namespace {
std::string get_device_type_string(VkPhysicalDeviceType type)
{
   switch (type) {
      case VK_PHYSICAL_DEVICE_TYPE_OTHER:
         return "Other";
      case VK_PHYSICAL_DEVICE_TYPE_INTEGRATED_GPU:
         return "Integrated GPU";
      case VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU:
         return "Discrete GPU";
      case VK_PHYSICAL_DEVICE_TYPE_VIRTUAL_GPU:
         return "Virtual GPU";
      case VK_PHYSICAL_DEVICE_TYPE_CPU:
         return "CPU";
      default:
         return "Unknown";
   }
}
}  // namespace

void Instance::log_device_info(Surface* surface,
                               const PhysicalDeviceRequirements& reqs,
                               bool with_extensions)
{
   if (!surface) {
      meddl::log::error("Can't log device info with a nullptr surface");
   }

   meddl::log::debug("Testing physical device requirements:");

   const auto& physical_devices = _physical_devices;
   for (const auto& device : physical_devices) {
      bool meets_req = device.meets_requirements(reqs, surface);
      int32_t score = device.score_device(reqs);

      meddl::log::debug("  Device: {}", device.get_properties().deviceName);
      meddl::log::debug("    Type: {}", get_device_type_string(device.get_properties().deviceType));
      meddl::log::debug("    Meets requirements: {}", meets_req ? "Yes" : "No");
      meddl::log::debug("    Score: {}", score);

      auto memory_props = device.get_memory_properties();
      VkDeviceSize total_memory = 0;

      meddl::log::debug("    Memory heaps:");
      for (uint32_t i = 0; i < memory_props.memoryHeapCount; i++) {
         bool is_device_local = memory_props.memoryHeaps[i].flags & VK_MEMORY_HEAP_DEVICE_LOCAL_BIT;
         total_memory += memory_props.memoryHeaps[i].size;

         meddl::log::debug("      Heap {}: {} MB ({})",
                           i,
                           memory_props.memoryHeaps[i].size / (1024 * 1024),  // NOLINT
                           is_device_local ? "Device Local" : "Host");
      }
      meddl::log::debug("    Total memory: {} MB", total_memory / (1024 * 1024));  // NOLINT

      const auto& queue_families = device.get_queue_families();
      meddl::log::debug("    Queue families: {}", queue_families.size());

      for (size_t i = 0; i < queue_families.size(); i++) {
         meddl::log::debug("      Family {}: {} queues", i, queue_families[i].queueCount);
         meddl::log::debug("        Graphics: {}",
                           (queue_families[i].queueFlags & VK_QUEUE_GRAPHICS_BIT) ? "Yes" : "No");
         meddl::log::debug("        Compute: {}",
                           (queue_families[i].queueFlags & VK_QUEUE_COMPUTE_BIT) ? "Yes" : "No");
         meddl::log::debug("        Transfer: {}",
                           (queue_families[i].queueFlags & VK_QUEUE_TRANSFER_BIT) ? "Yes" : "No");
         meddl::log::debug(
             "        Sparse: {}",
             (queue_families[i].queueFlags & VK_QUEUE_SPARSE_BINDING_BIT) ? "Yes" : "No");

         VkBool32 presentation_supported = VK_FALSE;
         if (surface) {
            vkGetPhysicalDeviceSurfaceSupportKHR(
                device.vk(), static_cast<uint32_t>(i), surface->vk(), &presentation_supported);
         }
         meddl::log::debug("        Presentation: {}", presentation_supported ? "Yes" : "No");
      }

      auto extensions = device.get_supported_extensions();
      meddl::log::debug("    Supported extensions: {}", extensions.size());
      meddl::log::debug("Device score: {}", device.score_device(reqs));

      meddl::log::debug("");  // Empty line for readability
   }
}
}  // namespace meddl::render::vk
