#pragma once

#include <volk.h>
#include <vector>
#include <glm/glm.hpp>
#include "nv_instance.h"
#include "nv_physicalDevice.h"
#include "nv_window.h"

namespace Nova {
    class Device {
    public:
        Device(Instance* instance, VkSurfaceKHR surface = VK_NULL_HANDLE);
        Device(Instance* instance, Window* window);
        ~Device();

        VkDevice GetRaw() const { return m_device; }
        VkSurfaceKHR GetSurface() const { return m_surface; }
        void SetSurface(VkSurfaceKHR surface) { m_surface = surface; }

        PhysicalDevice GetPhysicalDevice() const { return m_physicalDevices[m_selectedPhysicalDeviceIndex]; }
        const std::vector<PhysicalDevice> GetPhysicalDevices() const { return m_physicalDevices; }

        void SelectPhysicalDevice(size_t index);

        const VkSurfaceCapabilitiesKHR GetSurfaceCapabilities() const;
    private:
        Instance* m_instance;
        std::vector<PhysicalDevice> m_physicalDevices;
        size_t m_selectedPhysicalDeviceIndex;
        VkDevice m_device;
        VkSurfaceKHR m_surface;
    };
}