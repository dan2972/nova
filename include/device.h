#pragma once

#include <memory>

#include "backends/backend_device.h"

namespace Nova {
    enum class DeviceType {
        Vulkan,
    };

    class Device {
    public:
        Device() = default;
        ~Device() = default;
        void selectBackendDevice(DeviceType type);
    private:
        std::unique_ptr<BackendDevice> m_backendDevice;
    };

}