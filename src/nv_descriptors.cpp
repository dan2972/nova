#include "nv_descriptors.h"
#include "nv_device.h"
#include "nv_helpers.h"

#include <algorithm>

namespace nova {
    BindlessTextureDescriptors::BindlessTextureDescriptors(const Device& device, const std::vector<TextureDescriptor>& textureDescriptors)
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
            .stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT
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
                    .imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL
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
}
