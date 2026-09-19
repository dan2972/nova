#include "nv_sampler.h"

#include "nv_device.h"

namespace Nova {
    Sampler::Sampler(const Device &device, const SamplerCreateInfo &createInfo)
    {
        VkSamplerCreateInfo samplerCreateInfo{
            .sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO,
            .magFilter = static_cast<VkFilter>(createInfo.magFilter),
            .minFilter = static_cast<VkFilter>(createInfo.minFilter),
            .mipmapMode = static_cast<VkSamplerMipmapMode>(createInfo.mipmapMode),
            .addressModeU = static_cast<VkSamplerAddressMode>(createInfo.addressModeU),
            .addressModeV = static_cast<VkSamplerAddressMode>(createInfo.addressModeV),
            .addressModeW = static_cast<VkSamplerAddressMode>(createInfo.addressModeW),
            .mipLodBias = createInfo.mipLodBias,
            .anisotropyEnable = createInfo.anisotropyEnable ? VK_TRUE : VK_FALSE,
            .maxAnisotropy = createInfo.maxAnisotropy,
            .minLod = createInfo.minLod,
            .maxLod = createInfo.maxLod,
        };

        m_device = &device;
        CheckVk(vkCreateSampler(device.GetRaw(), &samplerCreateInfo, nullptr, &m_sampler));
    }

    Sampler::~Sampler()
    {
        Destroy();
    }

    void Sampler::Destroy()
    {
        if (m_sampler != VK_NULL_HANDLE && m_device != nullptr) {
            vkDestroySampler(m_device->GetRaw(), m_sampler, nullptr);
            m_sampler = VK_NULL_HANDLE;
            m_device = nullptr;
        }
    }

    Sampler::Sampler(Sampler&& other) noexcept
        : m_sampler(other.m_sampler), m_device(other.m_device)
    {
        other.m_sampler = VK_NULL_HANDLE;
        other.m_device = nullptr;
    }

    Sampler& Sampler::operator=(Sampler&& other) noexcept
    {
        if (this != &other) {
            Destroy();
            m_sampler = other.m_sampler;
            m_device = other.m_device;
            other.m_sampler = VK_NULL_HANDLE;
            other.m_device = nullptr;
        }
        return *this;
    }
}