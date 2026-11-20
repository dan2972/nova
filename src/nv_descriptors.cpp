#include "nv_descriptors.h"
#include "nv_device.h"
#include "nv_helpers.h"

#include <algorithm>
#include <stdexcept>
#include <unordered_set>

namespace nova {
    BindlessTextureDescriptors::BindlessTextureDescriptors(const Device& device, const std::vector<TextureDescriptor>& textureDescriptors,
                                                           ShaderStageFlag stageFlags)
        : m_device(&device) {

        static const VkDescriptorBindingFlags bindingFlags[] = {
            VK_DESCRIPTOR_BINDING_VARIABLE_DESCRIPTOR_COUNT_BIT
        };
        VkDescriptorSetLayoutBindingFlagsCreateInfo bindingFlagsCI{
            .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_BINDING_FLAGS_CREATE_INFO,
            .bindingCount = static_cast<uint32_t>(std::size(bindingFlags)),
            .pBindingFlags = bindingFlags
        };
        VkDescriptorSetLayoutBinding binding = {
            .binding = 0,
            .descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
            .descriptorCount = static_cast<uint32_t>(textureDescriptors.size()),
            .stageFlags = static_cast<VkShaderStageFlags>(stageFlags)
        };
        VkDescriptorSetLayoutCreateInfo layoutCI{
            .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO,
            .pNext = &bindingFlagsCI,
            .bindingCount = 1,
            .pBindings = &binding
        };
        CheckVk(vkCreateDescriptorSetLayout(device.GetRaw(), &layoutCI, nullptr, &m_descriptorSetLayout));

        VkDescriptorPoolSize poolSize = {
            .type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
            .descriptorCount = static_cast<uint32_t>(textureDescriptors.size())
        };
        VkDescriptorPoolCreateInfo descPoolCI{
            .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO,
            .maxSets = 1,
            .poolSizeCount = 1,
            .pPoolSizes = &poolSize
        };
        CheckVk(vkCreateDescriptorPool(device.GetRaw(), &descPoolCI, nullptr, &m_descriptorPool));

        uint32_t variableDescCount{ static_cast<uint32_t>(textureDescriptors.size()) };
        VkDescriptorSetVariableDescriptorCountAllocateInfo variableDescCountAI{
            .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_VARIABLE_DESCRIPTOR_COUNT_ALLOCATE_INFO_EXT,
            .descriptorSetCount = 1,
            .pDescriptorCounts = &variableDescCount
        };
        VkDescriptorSetAllocateInfo desctSetAI{
            .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO,
            .pNext = &variableDescCountAI,
            .descriptorPool = m_descriptorPool,
            .descriptorSetCount = 1,
            .pSetLayouts = &m_descriptorSetLayout
        };
        CheckVk(vkAllocateDescriptorSets(device.GetRaw(), &desctSetAI, &m_descriptorSet));

        std::vector<VkDescriptorImageInfo> imageInfos(textureDescriptors.size());
        std::transform(textureDescriptors.begin(), textureDescriptors.end(), imageInfos.begin(),
            [](const TextureDescriptor& desc) {
                VkDescriptorImageInfo imageInfo{
                    .sampler = desc.sampler->GetRaw(),
                    .imageView = desc.imageView->GetRaw(),
                    .imageLayout = VK_IMAGE_LAYOUT_READ_ONLY_OPTIMAL
                };
                return imageInfo;
            });
        VkWriteDescriptorSet writeDescriptorSet = {
            .sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
            .dstSet = m_descriptorSet,
            .dstBinding = 0,
            .descriptorCount = static_cast<uint32_t>(textureDescriptors.size()),
            .descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 
            .pImageInfo = imageInfos.data()
        };
        vkUpdateDescriptorSets(device.GetRaw(), 1, &writeDescriptorSet, 0, nullptr);
    }

    BindlessTextureDescriptors::~BindlessTextureDescriptors() {
        Destroy();
    }

    DescriptorSetLayout BindlessTextureDescriptors::GetDescriptorSetLayout() const
    {
        DescriptorSetLayout layout;
        layout.m_descriptorSetLayout = m_descriptorSetLayout;
        return layout;
    }

    DescriptorSet BindlessTextureDescriptors::GetDescriptorSet() const
    {
        DescriptorSet descriptorSet;
        descriptorSet.m_descriptorSet = m_descriptorSet;
        return descriptorSet;
    }

    void BindlessTextureDescriptors::Destroy()
    {
        if (m_descriptorSet != VK_NULL_HANDLE) {
            m_descriptorSet = VK_NULL_HANDLE;
        }
        if (m_descriptorPool != VK_NULL_HANDLE) {
            vkDestroyDescriptorPool(m_device->GetRaw(), m_descriptorPool, nullptr);
            m_descriptorPool = VK_NULL_HANDLE;
        }
        if (m_descriptorSetLayout != VK_NULL_HANDLE) {
            vkDestroyDescriptorSetLayout(m_device->GetRaw(), m_descriptorSetLayout, nullptr);
            m_descriptorSetLayout = VK_NULL_HANDLE;
        }
    }

    ResourceDescriptors::ResourceDescriptors(const Device& device, const std::vector<ResourceDescriptor>& resourceDescriptors)
        : m_device(&device)
    {
        if (resourceDescriptors.empty()) {
            throw std::invalid_argument("At least one resource descriptor is required.");
        }

        std::unordered_set<uint32_t> bindingIndices;
        std::vector<VkDescriptorSetLayoutBinding> bindings;
        std::vector<VkDescriptorPoolSize> poolSizes;
        std::vector<VkDescriptorBufferInfo> bufferInfos(resourceDescriptors.size());
        std::vector<VkDescriptorImageInfo> imageInfos(resourceDescriptors.size());
        std::vector<VkWriteDescriptorSet> writes;
        bindings.reserve(resourceDescriptors.size());
        writes.reserve(resourceDescriptors.size());

        for (size_t i = 0; i < resourceDescriptors.size(); ++i) {
            const ResourceDescriptor& descriptor = resourceDescriptors[i];
            if (!bindingIndices.insert(descriptor.binding).second || descriptor.stageFlags == ShaderStageFlag::None) {
                throw std::invalid_argument("Resource descriptors require unique bindings and at least one shader stage.");
            }

            const auto type = static_cast<VkDescriptorType>(descriptor.type);
            bindings.push_back({
                .binding = descriptor.binding,
                .descriptorType = type,
                .descriptorCount = 1,
                .stageFlags = static_cast<VkShaderStageFlags>(descriptor.stageFlags)
            });

            auto poolSize = std::ranges::find(poolSizes, type, &VkDescriptorPoolSize::type);
            if (poolSize == poolSizes.end()) {
                poolSizes.push_back({type, 1});
            } else {
                ++poolSize->descriptorCount;
            }

            VkWriteDescriptorSet& write = writes.emplace_back(VkWriteDescriptorSet{
                .sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
                .dstBinding = descriptor.binding,
                .descriptorCount = 1,
                .descriptorType = type
            });

            switch (descriptor.type) {
                case ResourceDescriptorType::UniformBuffer:
                case ResourceDescriptorType::StorageBuffer: {
                    const BufferUsageFlag usage = descriptor.type == ResourceDescriptorType::UniformBuffer
                        ? BufferUsageFlag::UniformBuffer : BufferUsageFlag::StorageBuffer;
                    if (!descriptor.buffer || (descriptor.buffer->GetUsageFlags() & usage) == BufferUsageFlag::None ||
                        descriptor.offset >= descriptor.buffer->GetSize() || descriptor.size == 0 ||
                        (descriptor.size != WholeBufferSize && descriptor.size > descriptor.buffer->GetSize() - descriptor.offset)) {
                        throw std::invalid_argument("Resource descriptor requires a buffer with matching usage and a valid range.");
                    }
                    bufferInfos[i] = {
                        .buffer = descriptor.buffer->GetRaw(),
                        .offset = descriptor.offset,
                        .range = descriptor.size
                    };
                    write.pBufferInfo = &bufferInfos[i];
                    break;
                }
                case ResourceDescriptorType::SampledImage:
                case ResourceDescriptorType::StorageImage:
                case ResourceDescriptorType::Sampler:
                case ResourceDescriptorType::CombinedImageSampler: {
                    const bool needsImage = descriptor.type != ResourceDescriptorType::Sampler;
                    const bool needsSampler = descriptor.type == ResourceDescriptorType::Sampler ||
                                              descriptor.type == ResourceDescriptorType::CombinedImageSampler;
                    if ((needsImage && (!descriptor.imageView || descriptor.imageView->GetRaw() == VK_NULL_HANDLE)) ||
                        (needsSampler && (!descriptor.sampler || descriptor.sampler->GetRaw() == VK_NULL_HANDLE))) {
                        throw std::invalid_argument("Resource descriptor requires a valid image view or sampler for its type.");
                    }
                    imageInfos[i] = {
                        .sampler = needsSampler ? descriptor.sampler->GetRaw() : VK_NULL_HANDLE,
                        .imageView = needsImage ? descriptor.imageView->GetRaw() : VK_NULL_HANDLE,
                        .imageLayout = descriptor.type == ResourceDescriptorType::StorageImage
                            ? VK_IMAGE_LAYOUT_GENERAL : VK_IMAGE_LAYOUT_READ_ONLY_OPTIMAL
                    };
                    write.pImageInfo = &imageInfos[i];
                    break;
                }
                default:
                    throw std::invalid_argument("Invalid resource descriptor type.");
            }
        }

        const VkDescriptorSetLayoutCreateInfo layoutCI{
            .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO,
            .bindingCount = static_cast<uint32_t>(bindings.size()),
            .pBindings = bindings.data()
        };
        CheckVk(vkCreateDescriptorSetLayout(device.GetRaw(), &layoutCI, nullptr, &m_descriptorSetLayout.m_descriptorSetLayout));

        const VkDescriptorPoolCreateInfo poolCI{
            .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO,
            .maxSets = 1,
            .poolSizeCount = static_cast<uint32_t>(poolSizes.size()),
            .pPoolSizes = poolSizes.data()
        };
        CheckVk(vkCreateDescriptorPool(device.GetRaw(), &poolCI, nullptr, &m_descriptorPool));

        const VkDescriptorSetAllocateInfo allocateInfo{
            .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO,
            .descriptorPool = m_descriptorPool,
            .descriptorSetCount = 1,
            .pSetLayouts = &m_descriptorSetLayout.GetRaw()
        };
        CheckVk(vkAllocateDescriptorSets(device.GetRaw(), &allocateInfo, &m_descriptorSet.m_descriptorSet));

        for (VkWriteDescriptorSet& write : writes) {
            write.dstSet = m_descriptorSet.GetRaw();
        }
        vkUpdateDescriptorSets(device.GetRaw(), static_cast<uint32_t>(writes.size()), writes.data(), 0, nullptr);
    }

    ResourceDescriptors& ResourceDescriptors::operator=(ResourceDescriptors&& other) noexcept
    {
        if (this != &other) {
            Destroy();
            m_device = std::exchange(other.m_device, nullptr);
            m_descriptorSetLayout = std::exchange(other.m_descriptorSetLayout, DescriptorSetLayout{});
            m_descriptorSet = std::exchange(other.m_descriptorSet, DescriptorSet{});
            m_descriptorPool = std::exchange(other.m_descriptorPool, VK_NULL_HANDLE);
        }
        return *this;
    }

    void ResourceDescriptors::Destroy()
    {
        if (m_descriptorPool != VK_NULL_HANDLE) {
            vkDestroyDescriptorPool(m_device->GetRaw(), m_descriptorPool, nullptr);
            m_descriptorPool = VK_NULL_HANDLE;
            m_descriptorSet = DescriptorSet{};
        }
        if (m_descriptorSetLayout.GetRaw() != VK_NULL_HANDLE) {
            vkDestroyDescriptorSetLayout(m_device->GetRaw(), m_descriptorSetLayout.GetRaw(), nullptr);
            m_descriptorSetLayout = DescriptorSetLayout{};
        }
        m_device = nullptr;
    }
}
