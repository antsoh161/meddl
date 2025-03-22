#include "engine/render/vk/physical_device.h"

#include <algorithm>
#include <limits>
#include <ratio>

#include "core/log.h"
#include "engine/render/vk/surface.h"

namespace meddl::render::vk {
PhysicalDevice::PhysicalDevice(Instance* instance, VkPhysicalDevice device)
    : _device(device), _instance(instance)
{
   vkGetPhysicalDeviceFeatures(_device, &_features);

   vkGetPhysicalDeviceProperties(_device, &_properties);

   uint32_t n_families = 0;
   vkGetPhysicalDeviceQueueFamilyProperties(_device, &n_families, nullptr);

   _queue_families.resize(n_families);
   vkGetPhysicalDeviceQueueFamilyProperties(_device, &n_families, _queue_families.data());

   // instance->getProcAddr(_vkGetPhysicalDeviceFeatures2,
   // "vkGetPhysicalDeviceFeatures2", "vkGetPhysicalDeviceFeatures2KHR");
   // instance->getProcAddr(_vkGetPhysicalDeviceProperties2,
   // "vkGetPhysicalDeviceProperties2", "vkGetPhysicalDeviceProperties2KHR");
}

PhysicalDevice::PhysicalDevice(PhysicalDevice&& other) noexcept
    : _device(other._device),
      _instance(other._instance),
      _features(other._features),
      _properties(other._properties),
      _queue_families(std::move(other._queue_families))
{
   other._device = VK_NULL_HANDLE;
   other._instance = nullptr;
}

PhysicalDevice& PhysicalDevice::operator=(PhysicalDevice&& other) noexcept
{
   if (this != &other) {
      _device = other._device;
      _instance = other._instance;
      _features = other._features;
      _properties = other._properties;
      _queue_families = std::move(other._queue_families);

      other._device = VK_NULL_HANDLE;
      other._instance = nullptr;
   }
   return *this;
}

const std::optional<uint32_t> PhysicalDevice::get_queue_family(VkQueueFlags flags) const
{
   for (int idx = 0; const auto& queue_family : _queue_families) {
      if ((queue_family.queueFlags & flags) == flags) {
         return idx;
      }
      idx++;
   }
   return {};
}

const std::optional<uint32_t> PhysicalDevice::get_present_family(Surface* surface) const
{
   VkBool32 has_present{false};
   for (int i = 0; i < _queue_families.size(); i++) {
      vkGetPhysicalDeviceSurfaceSupportKHR(_device, i, surface->vk(), &has_present);
      if (has_present) return i;
   }
   return {};
}

const std::vector<VkQueueFamilyProperties>& PhysicalDevice::get_queue_families() const
{
   return _queue_families;
}

VkPhysicalDeviceMemoryProperties PhysicalDevice::get_memory_properties() const
{
   VkPhysicalDeviceMemoryProperties props;
   vkGetPhysicalDeviceMemoryProperties(_device, &props);
   return props;
}
std::vector<VkExtensionProperties> PhysicalDevice::get_supported_extensions() const
{
   uint32_t extension_count = 0;
   vkEnumerateDeviceExtensionProperties(_device, nullptr, &extension_count, nullptr);
   std::vector<VkExtensionProperties> extensions(extension_count);
   vkEnumerateDeviceExtensionProperties(_device, nullptr, &extension_count, extensions.data());
   return extensions;
}

bool PhysicalDevice::has_extension_support(const std::string& extension_name) const
{
   auto extensions = get_supported_extensions();
   return std::ranges::any_of(extensions, [&extension_name](const VkExtensionProperties& props) {
      return extension_name == props.extensionName;
   });
}

bool PhysicalDevice::has_extensions_support(const std::unordered_set<std::string>& extensions) const
{
   auto supported_extensions = get_supported_extensions();

   std::unordered_set<std::string> supported_names;
   for (const auto& ext : supported_extensions) {
      supported_names.insert(ext.extensionName);
   }

   for (const auto& required_ext : extensions) {
      if (supported_names.find(required_ext) == supported_names.end()) {
         return false;
      }
   }
   return true;
}

bool PhysicalDevice::meets_requirements(const PhysicalDeviceRequirements& requirements,
                                        Surface* surface) const
{
   if (_properties.apiVersion < requirements.min_api_version) {
      meddl::log::error("Required API version is too high, requested minimum: {}, available: {}",
                        requirements.min_api_version,
                        _properties.apiVersion);
      return false;
   }

   if (!has_extensions_support(requirements.required_extensions)) {
      meddl::log::error("Required extensions not supported");
      return false;
   }

   for (auto required_queue_type : requirements.required_queue_types) {
      if (!get_queue_family(required_queue_type)) {
         meddl::log::error("Required queue types not supported");
         return false;
      }
   }

   if (requirements.requires_presentation && surface) {
      if (!get_present_family(surface)) {
         meddl::log::error("Required presentation family not supported");
         return false;
      }
   }

   if (requirements.minimum_memory_size > 0) {
      auto memory_props = get_memory_properties();
      VkDeviceSize total_memory = 0;
      for (uint32_t i = 0; i < memory_props.memoryHeapCount; i++) {
         const auto& heap = memory_props.memoryHeaps[i];
         if (heap.flags & VK_MEMORY_HEAP_DEVICE_LOCAL_BIT) {
            total_memory += heap.size;
         }
      }
      if (total_memory < requirements.minimum_memory_size) {
         meddl::log::error("Minimum required memory is too high");
         return false;
      }
   }

   if (requirements.required_features.has_value()) {
      // Compare each required feature with the available features
      const auto& req_features = requirements.required_features.value();

      // TODO: Annoying but works
      constexpr auto feature_checker = [](bool required, bool available) -> bool {
         return !required || available;
      };
#define CHECK_FEATURE(name) feature_checker(req_features.name, _features.name);

      CHECK_FEATURE(robustBufferAccess);
      CHECK_FEATURE(fullDrawIndexUint32);
      CHECK_FEATURE(imageCubeArray);
      CHECK_FEATURE(independentBlend);
      CHECK_FEATURE(geometryShader);
      CHECK_FEATURE(tessellationShader);
      CHECK_FEATURE(sampleRateShading);
      CHECK_FEATURE(dualSrcBlend);
      CHECK_FEATURE(logicOp);
      CHECK_FEATURE(multiDrawIndirect);
      CHECK_FEATURE(drawIndirectFirstInstance);
      CHECK_FEATURE(depthClamp);
      CHECK_FEATURE(depthBiasClamp);
      CHECK_FEATURE(fillModeNonSolid);
      CHECK_FEATURE(depthBounds);
      CHECK_FEATURE(wideLines);
      CHECK_FEATURE(largePoints);
      CHECK_FEATURE(alphaToOne);
      CHECK_FEATURE(multiViewport);
      CHECK_FEATURE(samplerAnisotropy);
      CHECK_FEATURE(textureCompressionETC2);
      CHECK_FEATURE(textureCompressionASTC_LDR);
      CHECK_FEATURE(textureCompressionBC);
      CHECK_FEATURE(occlusionQueryPrecise);
      CHECK_FEATURE(pipelineStatisticsQuery);
      CHECK_FEATURE(vertexPipelineStoresAndAtomics);
      CHECK_FEATURE(fragmentStoresAndAtomics);
      CHECK_FEATURE(shaderTessellationAndGeometryPointSize);
      CHECK_FEATURE(shaderImageGatherExtended);
      CHECK_FEATURE(shaderStorageImageExtendedFormats);
      CHECK_FEATURE(shaderStorageImageMultisample);
      CHECK_FEATURE(shaderStorageImageReadWithoutFormat);
      CHECK_FEATURE(shaderStorageImageWriteWithoutFormat);
      CHECK_FEATURE(shaderUniformBufferArrayDynamicIndexing);
      CHECK_FEATURE(shaderSampledImageArrayDynamicIndexing);
      CHECK_FEATURE(shaderStorageBufferArrayDynamicIndexing);
      CHECK_FEATURE(shaderStorageImageArrayDynamicIndexing);
      CHECK_FEATURE(shaderClipDistance);
      CHECK_FEATURE(shaderCullDistance);
      CHECK_FEATURE(shaderFloat64);
      CHECK_FEATURE(shaderInt64);
      CHECK_FEATURE(shaderInt16);
      CHECK_FEATURE(shaderResourceResidency);
      CHECK_FEATURE(shaderResourceMinLod);
      CHECK_FEATURE(sparseBinding);
      CHECK_FEATURE(sparseResidencyBuffer);
      CHECK_FEATURE(sparseResidencyImage2D);
      CHECK_FEATURE(sparseResidencyImage3D);
      CHECK_FEATURE(sparseResidency2Samples);
      CHECK_FEATURE(sparseResidency4Samples);
      CHECK_FEATURE(sparseResidency8Samples);
      CHECK_FEATURE(sparseResidency16Samples);
      CHECK_FEATURE(sparseResidencyAliased);
      CHECK_FEATURE(variableMultisampleRate);
      CHECK_FEATURE(inheritedQueries);

#undef CHECK_FEATURE
   }

   return true;
}

int32_t PhysicalDevice::score_device(const PhysicalDeviceRequirements& requirements) const
{
   // fail here = lowest possible score
   if (!meets_requirements(requirements)) {
      return std::numeric_limits<int32_t>::min();
   }
   if (requirements.device_scoring_function) {
      return requirements.device_scoring_function(this);
   }

   int32_t score = 0;

   constexpr int32_t type_score = 1000;

   //! We break on desired type, the order of the vector matters
   //! If first is not supported, we check next one and decrement score
   //! This weighs the highest
   for (size_t i = 0; i < requirements.device_type_preference.size(); i++) {
      if (_properties.deviceType == requirements.device_type_preference[i]) {
         score +=
             static_cast<int32_t>(type_score * (requirements.device_type_preference.size() - i));
         break;
      }
   }

   auto memory_props = get_memory_properties();
   constexpr int32_t gigabyte = 1024 * 1024 * 1024;

   //! If we meet requirements, +100 score
   constexpr int32_t memory_base_score = 100;

   VkDeviceSize total_device_local_memory = 0;
   for (uint32_t i = 0; i < memory_props.memoryHeapCount; i++) {
      const auto& heap = memory_props.memoryHeaps[i];
      if (heap.flags & VK_MEMORY_HEAP_DEVICE_LOCAL_BIT) {
         total_device_local_memory += heap.size;
      }
   }

   // First add base score if meeting minimum requirement
   if (total_device_local_memory >= requirements.minimum_memory_size) {
      score += memory_base_score;

      if (requirements.minimum_memory_size > 0) {
         // Calculate ratio of available memory to required memory
         // A device with 2x the minimum memory gets double the bonus points
         float memory_ratio = static_cast<float>(total_device_local_memory) /
                              static_cast<float>(requirements.minimum_memory_size);
         score += static_cast<int32_t>(memory_base_score * (memory_ratio - 1.0f));
      }
      else {
         score += static_cast<int32_t>(total_device_local_memory / gigabyte);
      }
   }

   constexpr int32_t shader_score = 10;
   constexpr int32_t tesselation_score = 10;
   constexpr int32_t sampler_score = 5;
   score += _features.geometryShader ? shader_score : 0;
   score += _features.tessellationShader ? tesselation_score : 0;
   score += _features.samplerAnisotropy ? sampler_score : 0;

   constexpr uint32_t nvidia_id = 0x10DE;

   // If I understand this correctly, its NVIDIA only
   // But newer drivers = higher score
   // if (_properties.vendorID == nvidia_id) {
   //  uint32_t major = (_properties.driverVersion >> 22) & 0x3ff;
   //  uint32_t minor = (_properties.driverVersion >> 14) & 0xff;
   //  uint32_t patch = (_properties.driverVersion >> 6) & 0xff;
   //  score += major * 100 + minor * 10 + patch;
   // }
   return score;
}

}  // namespace meddl::render::vk
