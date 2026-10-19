#pragma once

#include <volk.h>
#include <string>
#include <vector>

#include "nv_image.h"
#include "nv_surface.h"

namespace nova {
    class PhysicalDevice {
    public:
        PhysicalDevice(VkPhysicalDevice physicalDevice);
        PhysicalDevice(const PhysicalDevice&) = default;
        PhysicalDevice(PhysicalDevice&&) noexcept = default;
        PhysicalDevice& operator=(const PhysicalDevice&) = default;
        PhysicalDevice& operator=(PhysicalDevice&&) noexcept = default;
        ~PhysicalDevice() = default;

        VkPhysicalDevice GetRaw() const { return m_physicalDevice; }

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
        uint32_t GetGraphicsQueueFamilyIndex() const { return m_graphicsQueueFamilyIndex; }

        std::vector<VkQueueFamilyProperties> GetQueueFamilies() const { return m_queueFamilies; }
        std::vector<ImageFormat> GetSupportedDepthFormats() const;

        const VkSurfaceCapabilitiesKHR GetSurfaceCapabilities(Surface surface) const;

    private:
        VkPhysicalDevice m_physicalDevice;
        std::vector<VkQueueFamilyProperties> m_queueFamilies;
        uint32_t m_graphicsQueueFamilyIndex = 0;
        GPUType m_type;
        std::string m_deviceName;
        size_t m_vramSize;
    };
}
