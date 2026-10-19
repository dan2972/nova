#pragma once

#include <volk.h>
#include <string>
#include <vector>
#include "nv_physicalDevice.h"

namespace nova {
    class Instance {
    public:
        Instance(const std::string& applicationName = "nova Application");
        ~Instance();

        const std::vector<PhysicalDevice>& GetPhysicalDevices() const { return m_physicalDevices; }
        const std::vector<PhysicalDevice>& QueryPhysicalDevices();
        void SortPhysicalDevicesDefault(std::vector<PhysicalDevice>& physicalDevices) const;
        const PhysicalDevice& GetBestPhysicalDevice() const;

        VkInstance GetRaw() const { return m_instance; }
    private:
        void Init(const std::string& applicationName);

        VkInstance m_instance;
        VkDebugUtilsMessengerEXT m_debugMessenger;
        std::vector<PhysicalDevice> m_physicalDevices;

        friend class Device;
    };
}