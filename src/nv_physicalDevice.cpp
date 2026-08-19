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
}
