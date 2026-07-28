#include <stdexcept>

#include "device.h"
#include "backends/vk/vk_device.h"

namespace Nova {
    void Device::selectBackendDevice(DeviceType type) {
        switch (type) {
            case DeviceType::Vulkan:
                m_backendDevice = std::make_unique<vk::VulkanDevice>();
                break;
            default:
                throw std::runtime_error("Unsupported device type");
        }
    }

}