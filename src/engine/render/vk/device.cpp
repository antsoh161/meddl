#include "engine/render/vk/device.h"

#include <vulkan/vulkan_core.h>

#include <algorithm>
#include <vector>

#include "engine/render/vk/instance.h"
#include "engine/render/vk/physical_device.h"
#include "engine/render/vk/surface.h"

namespace meddl::render::vk {

//! Device
Device::Device(PhysicalDevice* physical_device,
               const std::unordered_map<uint32_t, QueueConfiguration>& queue_configurations,
               const std::unordered_set<std::string>& device_extensions,
               const std::optional<VkPhysicalDeviceFeatures>& device_features,
               const std::set<std::string>& validation_layers)
    : _physical_device(physical_device)
{
   std::vector<VkDeviceQueueCreateInfo> create_infos{};

   auto make_cinfo = [](uint32_t queue_family_index,
                        const QueueConfiguration& configuration) -> VkDeviceQueueCreateInfo {
      VkDeviceQueueCreateInfo queue_cinfo{};
      queue_cinfo.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
      queue_cinfo.queueFamilyIndex = queue_family_index;
      queue_cinfo.queueCount = configuration._queue_count;
      queue_cinfo.pQueuePriorities = &configuration._priority;
      return queue_cinfo;
   };
   for (const auto& config : queue_configurations) {
      create_infos.push_back(make_cinfo(config.first, config.second));
   }

   VkDeviceCreateInfo create_info{};
   create_info.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
   create_info.queueCreateInfoCount = create_infos.size();
   create_info.pQueueCreateInfos = create_infos.data();
   create_info.pEnabledFeatures = device_features.has_value() ? &device_features.value() : nullptr;
   create_info.enabledExtensionCount = static_cast<uint32_t>(device_extensions.size());

   std::vector<const char*> extensions_cstyle{};
   for (const auto& ext : device_extensions) {
      extensions_cstyle.push_back(ext.c_str());
   }
   create_info.ppEnabledExtensionNames = extensions_cstyle.data();

   std::vector<const char*> layers_cstyle{};
   if (validation_layers.empty()) {
      create_info.enabledLayerCount = validation_layers.size();
      for (const auto& layer : validation_layers) {
         layers_cstyle.push_back(layer.c_str());
      }
      create_info.ppEnabledLayerNames = layers_cstyle.data();
   }
   else {
      create_info.enabledLayerCount = 0;
      create_info.ppEnabledLayerNames = nullptr;
   }

   auto res = vkCreateDevice(_physical_device->vk(), &create_info, nullptr, &_device);
   if (res != VK_SUCCESS) {
      throw std::runtime_error{
          std::format("vkCreateDevice failed with error: {}", static_cast<int32_t>(res))};
   }

   for (auto& config : queue_configurations) {
      for (uint32_t i = 0; i < config.second._queue_count; i++) {
         VkQueue queue{};
         vkGetDeviceQueue(_device, config.first, i, &queue);
         _queues.emplace_back(queue, i, config.second);
      }
   }
}

Device::Device(PhysicalDevice* physical_device,
               const DeviceConfiguration& config,
               const std::optional<Debugger>& debugger)
    : _physical_device(physical_device)
{
   std::vector<VkDeviceQueueCreateInfo> create_infos{};

   auto make_cinfo = [](uint32_t queue_family_index,
                        const QueueConfiguration& configuration) -> VkDeviceQueueCreateInfo {
      VkDeviceQueueCreateInfo queue_cinfo{};
      queue_cinfo.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
      queue_cinfo.queueFamilyIndex = queue_family_index;
      queue_cinfo.queueCount = configuration._queue_count;
      queue_cinfo.pQueuePriorities = &configuration._priority;
      return queue_cinfo;
   };

   for (const auto& config_pair : config.queue_configurations) {
      create_infos.push_back(make_cinfo(config_pair.first, config_pair.second));
   }

   VkDeviceCreateInfo create_info{};
   create_info.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
   create_info.queueCreateInfoCount = static_cast<uint32_t>(create_infos.size());
   create_info.pQueueCreateInfos = create_infos.data();

   VkPhysicalDeviceFeatures device_features{};
   if (config.features.has_value()) {
      device_features = config.features.value();
   }
   else if (config.auto_enable_supported_features) {
      device_features = _physical_device->get_features();
   }
   create_info.pEnabledFeatures = &device_features;

   std::vector<const char*> extensions_cstyle{};
   for (const auto& ext : config.extensions) {
      extensions_cstyle.push_back(ext.c_str());
   }
   create_info.enabledExtensionCount = static_cast<uint32_t>(extensions_cstyle.size());
   create_info.ppEnabledExtensionNames = extensions_cstyle.data();

   std::vector<const char*> layers_cstyle{};
   if (debugger.has_value()) {
      for (const auto& layer : debugger->get_active_validation_layers()) {
         layers_cstyle.push_back(layer.c_str());
      }
   }
   create_info.enabledLayerCount = static_cast<uint32_t>(layers_cstyle.size());
   create_info.ppEnabledLayerNames = layers_cstyle.data();

   auto* last_structure = std::bit_cast<VkBaseOutStructure*>(&create_info);
   for (const auto& feature_pair : config.feature_chain) {
      auto structure = std::bit_cast<VkBaseOutStructure*>(feature_pair.second);
      structure->sType = feature_pair.first;
      last_structure->pNext = structure;
      last_structure = structure;
   }

   auto res =
       vkCreateDevice(_physical_device->vk(), &create_info, config.custom_allocator, &_device);
   if (res != VK_SUCCESS) {
      throw std::runtime_error{
          std::format("vkCreateDevice failed with error: {}", static_cast<int32_t>(res))};
   }

   _enabled_extensions = config.extensions;
   _enabled_features = device_features;

   for (auto& config_pair : config.queue_configurations) {
      for (uint32_t i = 0; i < config_pair.second._queue_count; i++) {
         VkQueue queue{};
         vkGetDeviceQueue(_device, config_pair.first, i, &queue);
         _queues.emplace_back(queue, i, config_pair.second);
      }
   }
}

Device::~Device()
{
   if (_device) {
      // TODO: Allocator
      vkDestroyDevice(_device, nullptr);
   }
}

Device::Device(Device&& other) noexcept
    : _queues(std::move(other._queues)),
      _device(other._device),
      _physical_device(other._physical_device)
{
   other._device = VK_NULL_HANDLE;
   other._physical_device = nullptr;
}

Device& Device::operator=(Device&& other) noexcept
{
   if (this != &other) {
      if (_device) {
         vkDestroyDevice(_device, nullptr);
      }

      _physical_device = other._physical_device;
      _device = other._device;
      _queues = std::move(other._queues);

      other._device = VK_NULL_HANDLE;
      other._physical_device = nullptr;
   }
   return *this;
}
void Device::wait_idle()
{
   if (vkDeviceWaitIdle(_device) != VK_SUCCESS) {
      throw std::runtime_error("Failed to wait for device idle");
   }
}

// Presets
namespace {
constexpr auto high_performance_features = []() {
   VkPhysicalDeviceFeatures features{};
   features.geometryShader = VK_TRUE;
   features.tessellationShader = VK_TRUE;
   features.samplerAnisotropy = VK_TRUE;
   features.fillModeNonSolid = VK_TRUE;
   features.multiViewport = VK_TRUE;
   return features;
}();
constexpr auto power_efficient_features = []() {
   VkPhysicalDeviceFeatures features{};
   // Minimal feature set to save power
   features.samplerAnisotropy = VK_TRUE;  // Basic quality of life feature
   return features;
}();

constexpr auto compute_focused_features = []() {
   VkPhysicalDeviceFeatures features{};
   features.shaderInt64 = VK_TRUE;
   features.shaderFloat64 = VK_TRUE;
   features.shaderStorageImageExtendedFormats = VK_TRUE;
   features.shaderStorageImageWriteWithoutFormat = VK_TRUE;
   return features;
}();

const std::vector<VkPhysicalDeviceType> high_performance_device_types = {
    VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU,
    VK_PHYSICAL_DEVICE_TYPE_INTEGRATED_GPU,
    VK_PHYSICAL_DEVICE_TYPE_VIRTUAL_GPU,
    VK_PHYSICAL_DEVICE_TYPE_CPU};

const std::vector<VkPhysicalDeviceType> power_efficient_device_types = {
    VK_PHYSICAL_DEVICE_TYPE_INTEGRATED_GPU,
    VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU,
    VK_PHYSICAL_DEVICE_TYPE_VIRTUAL_GPU,
    VK_PHYSICAL_DEVICE_TYPE_CPU};

const std::vector<VkPhysicalDeviceType> compute_focused_device_types = {
    VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU,
    VK_PHYSICAL_DEVICE_TYPE_VIRTUAL_GPU,
    VK_PHYSICAL_DEVICE_TYPE_INTEGRATED_GPU,
    VK_PHYSICAL_DEVICE_TYPE_CPU};
}  // namespace

DevicePicker::DevicePicker(Instance* instance, Surface* surface)
    : _instance(instance), _surface(surface)
{
}

DeviceConfiguration DevicePicker::create_config(PhysicalDevice* device,
                                                const PhysicalDeviceRequirements& reqs)
{
   DeviceConfiguration config;
   config.extensions = reqs.required_extensions;

   if (reqs.required_features.has_value()) {
      config.features = reqs.required_features;
   }

   std::unordered_map<uint32_t, QueueConfiguration> queue_configs;

   auto graphics_family = device->get_queue_family(VK_QUEUE_GRAPHICS_BIT);
   auto present_family = device->get_present_family(_surface);
   if (reqs.required_queue_types.find(VK_QUEUE_GRAPHICS_BIT) != reqs.required_queue_types.end()) {
      if (!graphics_family.has_value()) {
         meddl::log::error("Graphics family required, but not found");
         return {};  // Return empty config
      }
      queue_configs.emplace(graphics_family.value(),
                            QueueConfiguration(graphics_family.value(), 1.0f, 1));
      meddl::log::debug("Picked graphics family: {}, prio: 1.0, count: 1", graphics_family.value());
   }

   if (reqs.requires_presentation) {
      if (!present_family.has_value()) {
         meddl::log::error("Present family required, but not found");
         return {};
      }
      if (graphics_family.value() != present_family.value()) {
         queue_configs.emplace(present_family.value(),
                               QueueConfiguration(present_family.value(), 1.0f, 1));
         meddl::log::debug("Picked present family: {}, prio: 1.0, count: 1",
                           present_family.value());
      }
   }

   if (reqs.required_queue_types.find(VK_QUEUE_COMPUTE_BIT) != reqs.required_queue_types.end()) {
      auto compute_family = device->get_queue_family(VK_QUEUE_COMPUTE_BIT);
      if (compute_family.has_value()) {
         if (queue_configs.find(compute_family.value()) == queue_configs.end()) {
            queue_configs.emplace(compute_family.value(),
                                  QueueConfiguration(compute_family.value(), 1.0f, 1));
            meddl::log::debug("Picked compute family: {}, prio: 1.0, count: 1",
                              compute_family.value());
         }
      }
   }

   if (reqs.required_queue_types.find(VK_QUEUE_TRANSFER_BIT) != reqs.required_queue_types.end()) {
      auto transfer_family = device->get_queue_family(VK_QUEUE_TRANSFER_BIT);

      if (transfer_family.has_value()) {
         if (queue_configs.find(transfer_family.value()) == queue_configs.end()) {
            queue_configs.emplace(transfer_family.value(),
                                  QueueConfiguration(transfer_family.value(), 1.0f, 1));
            meddl::log::debug("Picked compute family: {}, prio: 1.0, count: 1",
                              transfer_family.value());
         }
      }
   }

   if (queue_configs.empty()) {
      meddl::log::error("Failed to configure any queue families for the device");
      return {};  // Return empty config
   }

   config.queue_configurations = std::move(queue_configs);
   return config;
};

std::optional<DevicePickerResult> DevicePicker::pick_best(DevicePickerStrategy strategy,
                                                          bool allow_best_effort)
{
   PhysicalDeviceRequirements reqs;

   reqs.required_extensions = {"VK_KHR_swapchain"};
   reqs.min_api_version = VK_API_VERSION_1_1;

   switch (strategy) {
      case DevicePickerStrategy::HighPerformance:
         reqs.required_queue_types = {
             VK_QUEUE_GRAPHICS_BIT, VK_QUEUE_COMPUTE_BIT, VK_QUEUE_TRANSFER_BIT};
         reqs.requires_presentation = true;
         reqs.device_type_preference = high_performance_device_types;
         reqs.required_features = high_performance_features;
         break;

      case DevicePickerStrategy::PowerEfficient:
         reqs.required_queue_types = {VK_QUEUE_GRAPHICS_BIT, VK_QUEUE_TRANSFER_BIT};
         reqs.requires_presentation = true;
         reqs.device_type_preference = power_efficient_device_types;
         reqs.required_features = power_efficient_features;
         break;

      case DevicePickerStrategy::ComputeFocused:
         reqs.required_queue_types = {VK_QUEUE_COMPUTE_BIT};
         reqs.requires_presentation = false;  // Compute doesn't necessarily need presentation
         reqs.device_type_preference = compute_focused_device_types;
         reqs.required_features = compute_focused_features;
         // Scoring function emphasizing compute capabilities
         break;
   }
   return pick_custom(reqs, allow_best_effort);
}

std::optional<DevicePickerResult> DevicePicker::pick_custom(PhysicalDeviceRequirements reqs,
                                                            bool allow_best_effort)
{
   auto& physical_devices = _instance->get_physical_devices();

   if (physical_devices.empty()) {
      meddl::log::error("No physical devices available for selection");
      return std::nullopt;
   }

   PhysicalDevice* best_device = nullptr;
   int32_t best_score = std::numeric_limits<int32_t>::min();
   bool strict_requirements_met = false;

   for (auto& device : physical_devices) {
      if (device.meets_requirements(reqs, _surface)) {
         int32_t score = device.score_device(reqs);
         meddl::log::debug("Device '{}' meets all requirements with score: {}",
                           device.get_properties().deviceName,
                           score);

         if (score > best_score) {
            best_score = score;
            best_device = &device;
            strict_requirements_met = true;
         }
      }
   }

   // Requirements not met, try best effort
   if (!strict_requirements_met && allow_best_effort) {
      meddl::log::warn("No device strictly meets all requirements, trying best-effort selection");

      // Create relaxed requirements (e.g., drop some features, keep mandatory queues)
      PhysicalDeviceRequirements relaxed_reqs = reqs;
      relaxed_reqs.required_features = std::nullopt;  // Drop feature requirements

      for (auto& device : physical_devices) {
         if (device.meets_requirements(relaxed_reqs, _surface)) {
            int32_t score = device.score_device(reqs);
            meddl::log::debug("Device '{}' meets relaxed requirements with score: {}",
                              device.get_properties().deviceName,
                              score);

            if (score > best_score) {
               best_score = score;
               best_device = &device;
            }
         }
      }
   }

   // If we still don't have a device, return nullopt
   if (!best_device) {
      meddl::log::error("Failed to find suitable device");
      return std::nullopt;
   }
   // Log the selected device info
   meddl::log::info(
       "Selected device: {} (score: {})", best_device->get_properties().deviceName, best_score);

   // Create device configuration based on the selected device
   DeviceConfiguration config = create_config(best_device, reqs);

   // Return both the device and its configuration
   return DevicePickerResult{.best_Device = best_device, .config = std::move(config)};
}

}  // namespace meddl::render::vk
