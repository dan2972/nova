#include "nv_texture.h"

#include "nv_device.h"
#include "nv_helpers.h"
#include "nv_commandBuffer.h"

namespace Nova {
    ImageView Texture::CreateView() const
    {
        return m_image.CreateView();
    }

    ImageView Texture::CreateView(ImageAspectFlag aspectMask) const
    {
        return m_image.CreateView(aspectMask);
    }

    Sampler Texture::CreateSampler(const SamplerCreateInfo &createInfo) const
    {
        return Sampler(*m_device, createInfo);
    }

    Sampler Texture::CreateLinearSampler() const
    {
        SamplerCreateInfo samplerInfo{
            .magFilter = SamplerFilter::Linear,
            .minFilter = SamplerFilter::Linear,
            .mipmapMode = SamplerMipmapMode::Linear,
            .mipLodBias = 0.0f,
            .anisotropyEnable = true,
            .maxAnisotropy = 8.0f,
            .maxLod = GetNumLevels() > 0 ? static_cast<float>(GetNumLevels()) : VK_LOD_CLAMP_NONE,
        };
        return CreateSampler(samplerInfo);
    }
}
