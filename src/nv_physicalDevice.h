#pragma once

#include <volk.h>
#include <string>

namespace Nova {
    class PhysicalDevice {
    public:
        PhysicalDevice(VkPhysicalDevice physicalDevice);
        PhysicalDevice(const PhysicalDevice&) = default;
        PhysicalDevice(PhysicalDevice&&) noexcept = default;
        PhysicalDevice& operator=(const PhysicalDevice&) = default;
        PhysicalDevice& operator=(PhysicalDevice&&) noexcept = default;
        ~PhysicalDevice() = default;

        VkPhysicalDevice Get() const { return m_physicalDevice; }

        enum class GPUType : uint32_t {
            Other,
            IntegratedGPU,
            DiscreteGPU,
            VirtualGPU,
            CPU
        };

        GPUType VkTypeToNovaType(VkPhysicalDeviceType vkType) const;
        GPUType GetType() const { return m_type; }
        std::string GetDeviceName() const { return m_deviceName; }
        size_t GetVRAMSize() const { return m_vramSize; }

    private:
        VkPhysicalDevice m_physicalDevice;
        GPUType m_type;
        std::string m_deviceName;
        size_t m_vramSize;
    };
}
