#include "nv_physicalDevice.h"

namespace Nova {
    PhysicalDevice::PhysicalDevice(VkPhysicalDevice physicalDevice)
        : m_physicalDevice(physicalDevice)
    {
        VkPhysicalDeviceProperties properties;
        vkGetPhysicalDeviceProperties(m_physicalDevice, &properties);
        m_type = VkTypeToNovaType(properties.deviceType);
        m_deviceName = properties.deviceName;

        VkPhysicalDeviceMemoryProperties memoryProperties;
        vkGetPhysicalDeviceMemoryProperties(m_physicalDevice, &memoryProperties);
        m_vramSize = 0;
        for (uint32_t i = 0; i < memoryProperties.memoryHeapCount; ++i) {
            if (memoryProperties.memoryHeaps[i].flags & VK_MEMORY_HEAP_DEVICE_LOCAL_BIT) {
                m_vramSize += memoryProperties.memoryHeaps[i].size;
            }
        }

        uint32_t queueFamilyCount = 0;
        vkGetPhysicalDeviceQueueFamilyProperties(physicalDevice, &queueFamilyCount, nullptr);
        m_queueFamilies.resize(queueFamilyCount);
        vkGetPhysicalDeviceQueueFamilyProperties(physicalDevice, &queueFamilyCount, m_queueFamilies.data());
        for (uint32_t i = 0; i < queueFamilyCount; i++) {
            if (m_queueFamilies[i].queueFlags & VK_QUEUE_GRAPHICS_BIT) {
                m_graphicsQueueFamilyIndex = i;
                break;
            }
        }
    }

    PhysicalDevice::GPUType PhysicalDevice::VkTypeToNovaType(VkPhysicalDeviceType vkType) const
    {
        switch (vkType) {
            case VK_PHYSICAL_DEVICE_TYPE_OTHER:
                return GPUType::Other;
            case VK_PHYSICAL_DEVICE_TYPE_INTEGRATED_GPU:
                return GPUType::IntegratedGPU;
            case VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU:
                return GPUType::DiscreteGPU;
            case VK_PHYSICAL_DEVICE_TYPE_VIRTUAL_GPU:
                return GPUType::VirtualGPU;
            case VK_PHYSICAL_DEVICE_TYPE_CPU:
                return GPUType::CPU;
            default:
                return GPUType::Other;
        }
    }

    std::vector<ImageFormat> PhysicalDevice::GetSupportedDepthFormats() const
    {
        static const std::vector<VkFormat> depthFormats = {
            VK_FORMAT_D32_SFLOAT_S8_UINT,
            VK_FORMAT_D24_UNORM_S8_UINT
        };

        std::vector<ImageFormat> supportedDepthFormats;
        for (const auto& format : depthFormats) {
            VkFormatProperties2 formatProperties{ .sType = VK_STRUCTURE_TYPE_FORMAT_PROPERTIES_2 };
            vkGetPhysicalDeviceFormatProperties2(m_physicalDevice, format, &formatProperties);
            if (formatProperties.formatProperties.optimalTilingFeatures & VK_FORMAT_FEATURE_DEPTH_STENCIL_ATTACHMENT_BIT) {
                supportedDepthFormats.push_back(static_cast<ImageFormat>(format));
            }
        }
        return supportedDepthFormats;
    }

    const VkSurfaceCapabilitiesKHR PhysicalDevice::GetSurfaceCapabilities(Surface surface) const
    {
        if (surface.GetRaw() == VK_NULL_HANDLE) {
            LogError("Attempted to get surface capabilities with a null surface.");
            exit(-1);
        }
        VkSurfaceCapabilitiesKHR capabilities;
        vkGetPhysicalDeviceSurfaceCapabilitiesKHR(m_physicalDevice, surface.GetRaw(), &capabilities);
        return capabilities;
    }
}
