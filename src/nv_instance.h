#pragma once

#include <volk.h>
#include <string>
#include <vector>
#include "nv_physicalDevice.h"

namespace Nova {
    class Instance {
    public:
        Instance(std::string applicationName = "Nova Application");
        ~Instance();

        const std::vector<PhysicalDevice>& GetPhysicalDevices() const { return m_physicalDevices; }
        const std::vector<PhysicalDevice>& QueryPhysicalDevices();
        void SortPhysicalDevicesDefault(std::vector<PhysicalDevice>& physicalDevices) const;
        const PhysicalDevice* GetBestPhysicalDevice() const;

        VkInstance GetRaw() const { return m_instance; }
    private:
        void Init(std::string applicationName);

        VkInstance m_instance;
        VkDebugUtilsMessengerEXT m_debugMessenger;
        std::vector<PhysicalDevice> m_physicalDevices;

        friend class Device;
    };
}