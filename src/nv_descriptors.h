#pragma once

#include <volk.h>
#include <utility>
#include <vector>
#include "nv_buffer.h"
#include "nv_texture.h"
#include "nv_image.h"
#include "nv_sampler.h"
#include "nv_shader.h"

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
        friend class ResourceDescriptors;
    };

    class DescriptorSet {
    public:
        DescriptorSet() = default;

        const VkDescriptorSet& GetRaw() const { return m_descriptorSet; }
    private:
        VkDescriptorSet m_descriptorSet = VK_NULL_HANDLE;

        friend class BindlessTextureDescriptors;
        friend class ResourceDescriptors;
    };

    class BindlessTextureDescriptors {
    public:
        BindlessTextureDescriptors(const Device& device, const std::vector<TextureDescriptor>& textureDescriptors,
                                   ShaderStageFlag stageFlags = ShaderStageFlag::Fragment);
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

    enum class ResourceDescriptorType : std::underlying_type_t<VkDescriptorType> {
        UniformBuffer = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
        StorageBuffer = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER,
        SampledImage = VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE,
        StorageImage = VK_DESCRIPTOR_TYPE_STORAGE_IMAGE,
        Sampler = VK_DESCRIPTOR_TYPE_SAMPLER,
        CombinedImageSampler = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER
    };

    struct ResourceDescriptor {
        uint32_t binding = 0;
        ResourceDescriptorType type = ResourceDescriptorType::StorageBuffer;
        ShaderStageFlag stageFlags = ShaderStageFlag::Compute;
        const Buffer* buffer = nullptr;
        BufferSize offset = 0;
        BufferSize size = WholeBufferSize;
        const ImageView* imageView = nullptr;
        const Sampler* sampler = nullptr;
    };

    class ResourceDescriptors {
    public:
        ResourceDescriptors(const Device& device, const std::vector<ResourceDescriptor>& resourceDescriptors);
        ~ResourceDescriptors() { Destroy(); }

        ResourceDescriptors(const ResourceDescriptors&) = delete;
        ResourceDescriptors& operator=(const ResourceDescriptors&) = delete;
        ResourceDescriptors(ResourceDescriptors&& other) noexcept { *this = std::move(other); }
        ResourceDescriptors& operator=(ResourceDescriptors&& other) noexcept;

        DescriptorSetLayout GetDescriptorSetLayout() const { return m_descriptorSetLayout; }
        DescriptorSet GetDescriptorSet() const { return m_descriptorSet; }

        void Destroy();

    private:
        const Device* m_device = nullptr;
        DescriptorSetLayout m_descriptorSetLayout;
        DescriptorSet m_descriptorSet;
        VkDescriptorPool m_descriptorPool = VK_NULL_HANDLE;
    };
}
