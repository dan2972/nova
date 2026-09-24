#pragma once

#include <volk.h>
#include <vector>
#include "nv_texture.h"
#include "nv_image.h"
#include "nv_sampler.h"

namespace Nova {
    class Device;

    struct TextureDescriptor {
        const ImageView* imageView;
        const Sampler* sampler;
    };

    class BindlessTextureDescriptors {
    public:
        BindlessTextureDescriptors(const Device& device, const std::vector<TextureDescriptor>& textureDescriptors);
        ~BindlessTextureDescriptors();

        BindlessTextureDescriptors(const BindlessTextureDescriptors&) = delete;
        BindlessTextureDescriptors& operator=(const BindlessTextureDescriptors&) = delete;

        void Destroy();
    private:
        const Device* m_device = nullptr;

        VkDescriptorSet m_descriptorSet = VK_NULL_HANDLE;
        VkDescriptorPool m_descriptorPool = VK_NULL_HANDLE;
        VkDescriptorSetLayout m_descriptorSetLayout = VK_NULL_HANDLE;
    };
}