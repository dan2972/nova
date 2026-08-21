#pragma once

#include <volk.h>
#include <vector>
#include "nv_instance.h"
#include "nv_physicalDevice.h"

namespace Nova {
    class Device {
    public:
        Device(Instance* instance);
        ~Device();

        VkDevice GetRaw() const { return m_device; }

        PhysicalDevice GetPhysicalDevice() const { return m_physicalDevices[m_selectedPhysicalDeviceIndex]; }
        const std::vector<PhysicalDevice> GetPhysicalDevices() const { return m_physicalDevices; }

        void SelectPhysicalDevice(size_t index);

    private:
        Instance* m_instance;
        std::vector<PhysicalDevice> m_physicalDevices;
        size_t m_selectedPhysicalDeviceIndex;
        VkDevice m_device;
    };
}