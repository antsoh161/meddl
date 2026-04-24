#pragma once

#include <vulkan/vulkan_core.h>

#include <functional>
#include <optional>
#include <string>
#include <unordered_set>
#include <vector>
namespace meddl::render::vk {

class PhysicalDevice;
struct PhysicalDeviceRequirements {
   std::optional<VkPhysicalDeviceFeatures> required_features{};
   std::unordered_set<std::string> required_extensions{};
   std::unordered_set<VkQueueFlagBits> required_queue_types{};
   VkDeviceSize minimum_memory_size{0};
   std::vector<VkPhysicalDeviceType> device_type_preference{VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU,
                                                            VK_PHYSICAL_DEVICE_TYPE_INTEGRATED_GPU,
                                                            VK_PHYSICAL_DEVICE_TYPE_VIRTUAL_GPU,
                                                            VK_PHYSICAL_DEVICE_TYPE_CPU};
   std::function<int32_t(const PhysicalDevice*)> device_scoring_function{nullptr};
   bool requires_presentation{true};
   uint32_t min_api_version{VK_API_VERSION_1_0};
};

class Instance;
class Surface;
class PhysicalDevice {
  public:
   PhysicalDevice(Instance* instance, VkPhysicalDevice device);
   ~PhysicalDevice() = default;

   PhysicalDevice(const PhysicalDevice&) = delete;
   PhysicalDevice& operator=(const PhysicalDevice&) = delete;
   PhysicalDevice(PhysicalDevice&& other) noexcept;
   PhysicalDevice& operator=(PhysicalDevice&& other) noexcept;

   [[nodiscard]] VkPhysicalDevice vk() const { return _device; }

   Instance* instance() { return _instance; }

   //! @brief
   //! Returns the queue family only if it's a perfect match to flags
   [[nodiscard]] const std::optional<uint32_t> get_queue_family(VkQueueFlags flags) const;
   [[nodiscard]] const std::optional<uint32_t> get_present_family(Surface* surface) const;
   [[nodiscard]] const std::vector<VkQueueFamilyProperties>& get_queue_families() const;
   [[nodiscard]] const std::vector<VkSurfaceFormatKHR> formats(Surface* surface) const;
   [[nodiscard]] const std::vector<VkPresentModeKHR> present_modes(Surface* surface) const;
   [[nodiscard]] const VkSurfaceCapabilitiesKHR capabilities(Surface* surface) const;

   [[nodiscard]] const VkPhysicalDeviceProperties& get_properties() const { return _properties; }
   [[nodiscard]] const VkPhysicalDeviceFeatures& get_features() const { return _features; };
   [[nodiscard]] VkPhysicalDeviceMemoryProperties get_memory_properties() const;
   [[nodiscard]] std::vector<VkExtensionProperties> get_supported_exstensions() const;
   [[nodiscard]] bool has_extension_support(const std::string& extension_name) const;
   [[nodiscard]] bool has_extensions_support(
       const std::unordered_set<std::string>& extensions) const;
   [[nodiscard]] std::vector<VkExtensionProperties> get_supported_extensions() const;
   [[nodiscard]] bool meets_requirements(const PhysicalDeviceRequirements& requirements,
                                         Surface* surface = nullptr) const;
   //! This scoring is pretty arbitrary, but works
   [[nodiscard]] int32_t score_device(const PhysicalDeviceRequirements& requirements) const;

  private:
   VkPhysicalDevice _device;
   Instance* _instance;

   VkPhysicalDeviceFeatures _features{};
   VkPhysicalDeviceProperties _properties{};
   std::vector<VkQueueFamilyProperties> _queue_families{};
   PFN_vkGetPhysicalDeviceFeatures2 _vkGetPhysicalDeviceFeatures2 = nullptr;
   PFN_vkGetPhysicalDeviceProperties2 _vkGetPhysicalDeviceProperties2 = nullptr;
};
}  // namespace meddl::render::vk
