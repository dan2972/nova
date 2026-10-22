#pragma once

#include <volk.h>
#include <vector>
#include "nv_texture.h"
#include "nv_image.h"
#include "nv_sampler.h"

namespace nova {
    class Device;

    struct TextureDescriptor {
        const ImageView* imageView;
        const Sampler* sampler;
    };

    class DescriptorSetLayout {
    public:
        DescriptorSetLayout() = default;

        const VkDescriptorSetLayout& GetRaw() const { return m_descriptorSetLayout; }
    
    private:
        VkDescriptorSetLayout m_descriptorSetLayout = VK_NULL_HANDLE;

        friend class BindlessTextureDescriptors;
    };

    class DescriptorSet {
    public:
        DescriptorSet() = default;

        const VkDescriptorSet& GetRaw() const { return m_descriptorSet; }
    private:
        VkDescriptorSet m_descriptorSet = VK_NULL_HANDLE;

        friend class BindlessTextureDescriptors;
    };

    class BindlessTextureDescriptors {
    public:
        BindlessTextureDescriptors(const Device& device, const std::vector<TextureDescriptor>& textureDescriptors);
        ~BindlessTextureDescriptors();

        BindlessTextureDescriptors(const BindlessTextureDescriptors&) = delete;
        BindlessTextureDescriptors& operator=(const BindlessTextureDescriptors&) = delete;
        BindlessTextureDescriptors(BindlessTextureDescriptors&& other) noexcept { *this = std::move(other); }
        BindlessTextureDescriptors& operator=(BindlessTextureDescriptors&& other) noexcept {
            if (this != &other) {
                Destroy();
                m_device = other.m_device;
                m_descriptorSet = other.m_descriptorSet;
                m_descriptorPool = other.m_descriptorPool;
                m_descriptorSetLayout = other.m_descriptorSetLayout;
                other.m_device = nullptr;
                other.m_descriptorSet = VK_NULL_HANDLE;
                other.m_descriptorPool = VK_NULL_HANDLE;
                other.m_descriptorSetLayout = VK_NULL_HANDLE;
            }
            return *this;
        }

        DescriptorSetLayout GetDescriptorSetLayout() const;
        DescriptorSet GetDescriptorSet() const;

        void Destroy();


    private:
        const Device* m_device = nullptr;

        VkDescriptorSet m_descriptorSet = VK_NULL_HANDLE;
        VkDescriptorPool m_descriptorPool = VK_NULL_HANDLE;
        VkDescriptorSetLayout m_descriptorSetLayout = VK_NULL_HANDLE;
    };
}