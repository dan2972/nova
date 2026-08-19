
#include <algorithm>
#include "nv_helpers.h"
#include "nv_device.h"

namespace {
    void sGetPhysicalDevices(VkInstance instance, std::vector<Nova::PhysicalDevice>& physicalDevices)
    {
        uint32_t deviceCount = 0;
        std::vector<VkPhysicalDevice> physicalDevicesRaw;
        CheckVk(vkEnumeratePhysicalDevices(instance, &deviceCount, nullptr));
        physicalDevicesRaw.resize(deviceCount);
        CheckVk(vkEnumeratePhysicalDevices(instance, &deviceCount, physicalDevicesRaw.data()));
        physicalDevices.reserve(deviceCount);
        for (uint32_t i = 0; i < deviceCount; ++i) {
            physicalDevices.emplace_back(physicalDevicesRaw[i]);
        }
    }

    // sort by device type, preferring discrete GPUs,
    // then by amount of VRAM, preferring more VRAM
    void sSortPhysicalDevices(std::vector<Nova::PhysicalDevice>& physicalDevices)
    {
        std::sort(physicalDevices.begin(), physicalDevices.end(),
            [](Nova::PhysicalDevice a, Nova::PhysicalDevice b) {
                if (a.GetType() != b.GetType()) {
                    return a.GetType() == Nova::PhysicalDevice::GPUType::DiscreteGPU;
                }

                return a.GetVRAMSize() > b.GetVRAMSize();
            });
    }
}

namespace Nova {
    Device::Device(Instance* instance)
        : m_instance(instance)
    {
        sGetPhysicalDevices(m_instance->m_instance, m_physicalDevices);
        sSortPhysicalDevices(m_physicalDevices);

        if (m_physicalDevices.empty()) {
            spdlog::error("No GPU found.");
            exit(-1);
        }

        m_selectedPhysicalDeviceIndex = 0;
        
        const PhysicalDevice& selectedDevice = m_physicalDevices[m_selectedPhysicalDeviceIndex];
        spdlog::info("Selected GPU: {} (type: {}, VRAM: {} MB)",
            selectedDevice.GetDeviceName(),
            static_cast<int>(selectedDevice.GetType()),
            selectedDevice.GetVRAMSize() / (1024 * 1024));
    }

    Device::~Device()
    {
    }
}
