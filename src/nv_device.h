#pragma once

#include <volk.h>
#include <vector>
#include <glm/glm.hpp>
#include "nv_instance.h"
#include "nv_physicalDevice.h"
#include "nv_window.h"

namespace Nova {
    enum class DepthFormat {
        D16Unorm = VK_FORMAT_D16_UNORM,
        D32Sfloat = VK_FORMAT_D32_SFLOAT,
        D24UnormS8Uint = VK_FORMAT_D24_UNORM_S8_UINT,
        D32SfloatS8Uint = VK_FORMAT_D32_SFLOAT_S8_UINT
    };

    class Device {
    public:
        Device(Instance* instance, VkSurfaceKHR surface = VK_NULL_HANDLE);
        Device(Instance* instance, Window* window);
        ~Device();

        Device(const Device&) = delete;
        Device& operator=(const Device&) = delete;
        Device(Device&&) = delete;
        Device& operator=(Device&&) = delete;

        VkDevice GetRaw() const { return m_device; }
        VkSurfaceKHR GetSurface() const { return m_surface; }
        void SetSurface(VkSurfaceKHR surface) { m_surface = surface; }

        PhysicalDevice GetPhysicalDevice() const { return m_physicalDevices[m_selectedPhysicalDeviceIndex]; }
        const std::vector<PhysicalDevice> GetPhysicalDevices() const { return m_physicalDevices; }

        void SelectPhysicalDevice(size_t index);

        const VkSurfaceCapabilitiesKHR GetSurfaceCapabilities() const;

        std::vector<DepthFormat> GetSupportedDepthFormats() const;
    private:
        Instance* m_instance;
        std::vector<PhysicalDevice> m_physicalDevices;
        size_t m_selectedPhysicalDeviceIndex;
        VkDevice m_device = VK_NULL_HANDLE;
        VkSurfaceKHR m_surface = VK_NULL_HANDLE;
    };
}
