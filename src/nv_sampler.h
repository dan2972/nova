#pragma once

#include <volk.h>

namespace Nova {
    class Device;

    enum class SamplerFilter {
        Nearest = VK_FILTER_NEAREST,
        Linear = VK_FILTER_LINEAR
    };

    enum class SamplerAddressMode {
        Repeat = VK_SAMPLER_ADDRESS_MODE_REPEAT,
        MirroredRepeat = VK_SAMPLER_ADDRESS_MODE_MIRRORED_REPEAT,
        ClampToEdge = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE,
        ClampToBorder = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_BORDER
    };

    enum class SamplerMipmapMode {
        Nearest = VK_SAMPLER_MIPMAP_MODE_NEAREST,
        Linear = VK_SAMPLER_MIPMAP_MODE_LINEAR
    };

    struct SamplerCreateInfo {
        SamplerFilter magFilter = SamplerFilter::Linear;
        SamplerFilter minFilter = SamplerFilter::Linear;
        SamplerMipmapMode mipmapMode = SamplerMipmapMode::Linear;
        SamplerAddressMode addressModeU = SamplerAddressMode::Repeat;
        SamplerAddressMode addressModeV = SamplerAddressMode::Repeat;
        SamplerAddressMode addressModeW = SamplerAddressMode::Repeat;
        float mipLodBias = 0.0f;
        bool anisotropyEnable = true;
        float maxAnisotropy = 16.0f;
        float minLod = 0.0f;
        float maxLod = VK_LOD_CLAMP_NONE;
    };
    
    class Sampler {
    public:
        Sampler() = default;
        Sampler(const Device& device, const SamplerCreateInfo& createInfo);
        ~Sampler();

        void Destroy();

        Sampler(const Sampler&) = delete;
        Sampler& operator=(const Sampler&) = delete;

        Sampler(Sampler&& other) noexcept;
        Sampler& operator=(Sampler&& other) noexcept;

        VkSampler GetRaw() const { return m_sampler; }
    private:
        VkSampler m_sampler{ VK_NULL_HANDLE };
        const Device* m_device{ nullptr };

        friend class Texture;
    };
}