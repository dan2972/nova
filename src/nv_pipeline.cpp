#include "nv_pipeline.h"
#include "nv_device.h"

namespace Nova {
    GraphicsPipelineBuilder &GraphicsPipelineBuilder::AddPushConstantRange(const PushConstantRange &pushConstantRange)
    {
        m_pushConstantRanges.push_back(
            {
                .stageFlags = static_cast<VkShaderStageFlags>(pushConstantRange.stageFlags),
                .offset = pushConstantRange.offset,
                .size = pushConstantRange.size
            }
        );
        return *this;
    }

    GraphicsPipelineBuilder &GraphicsPipelineBuilder::AddDescriptorSetLayout(const DescriptorSetLayout &descriptorSetLayout)
    {
        m_descriptorSetLayouts.push_back(descriptorSetLayout.GetRaw());
        return *this;
    }

    GraphicsPipelineBuilder &GraphicsPipelineBuilder::AddVertexBindingLayout(const VertexBindingLayout &vertexBindingLayout)
    {
        m_vertexInputBindingDescriptions.push_back({
            .binding = vertexBindingLayout.binding,
            .stride = vertexBindingLayout.stride,
            .inputRate = VK_VERTEX_INPUT_RATE_VERTEX
        });

        for (const auto& attribute : vertexBindingLayout.attributes) {
            m_vertexInputAttributeDescriptions.push_back({
                .location = static_cast<uint32_t>(m_vertexInputAttributeDescriptions.size()),
                .binding = vertexBindingLayout.binding,
                .format = static_cast<VkFormat>(attribute.format),
                .offset = attribute.offset
            });
        }

        return *this;
    }

    GraphicsPipelineBuilder &GraphicsPipelineBuilder::AddShaderStage(ShaderStageFlag stage, const ShaderModule& shaderModule, const std::string& entryPoint)
    {
        m_shaderStageEntryPoints.push_back(entryPoint);
        m_shaderStages.push_back({
            .sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO,
            .stage = static_cast<VkShaderStageFlagBits>(stage),
            .module = shaderModule.GetRaw(),
            .pName = m_shaderStageEntryPoints.back().c_str()
        });
        return *this;
    }

    GraphicsPipelineBuilder &GraphicsPipelineBuilder::EnableDepthStencil(bool depthTestEnable, bool depthWriteEnable, ImageFormat depthFormat)
    {
        m_depthTestEnable = depthTestEnable;
        m_depthWriteEnable = depthWriteEnable;
        m_depthFormat = depthFormat;
        return *this;
    }

    GraphicsPipeline GraphicsPipelineBuilder::Build()
    {
        uint32_t pushConstantCount = static_cast<uint32_t>(m_pushConstantRanges.size());
        uint32_t descriptorSetLayoutCount = static_cast<uint32_t>(m_descriptorSetLayouts.size());
        VkPipelineLayoutCreateInfo pipelineLayoutCreateInfo = {
            .sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO,
            .setLayoutCount = descriptorSetLayoutCount,
            .pSetLayouts = descriptorSetLayoutCount > 0 ? m_descriptorSetLayouts.data() : nullptr,
            .pushConstantRangeCount = pushConstantCount,
            .pPushConstantRanges = pushConstantCount > 0 ? m_pushConstantRanges.data() : nullptr
        };

        VkPipelineLayout pipelineLayout;
        CheckVk(vkCreatePipelineLayout(m_device.GetRaw(), &pipelineLayoutCreateInfo, nullptr, &pipelineLayout));

        VkPipelineVertexInputStateCreateInfo vertexInputState{
            .sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO,
            .vertexBindingDescriptionCount = static_cast<uint32_t>(m_vertexInputBindingDescriptions.size()),
            .pVertexBindingDescriptions = m_vertexInputBindingDescriptions.data(),
            .vertexAttributeDescriptionCount = static_cast<uint32_t>(m_vertexInputAttributeDescriptions.size()),
            .pVertexAttributeDescriptions = m_vertexInputAttributeDescriptions.data()
        };

        VkPipelineInputAssemblyStateCreateInfo inputAssemblyState{
            .sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO,
            .topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST
        };

        VkPipelineViewportStateCreateInfo viewportState{
            .sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO,
            .viewportCount = 1,
            .scissorCount = 1
        };
        std::vector<VkDynamicState> dynamicStates{ VK_DYNAMIC_STATE_VIEWPORT, VK_DYNAMIC_STATE_SCISSOR };
        VkPipelineDynamicStateCreateInfo dynamicState{
            .sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO,
            .dynamicStateCount = 2,
            .pDynamicStates = dynamicStates.data()
        };

        VkPipelineDepthStencilStateCreateInfo depthStencilState{
            .sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO,
            .depthTestEnable = m_depthTestEnable ? VK_TRUE : VK_FALSE,
            .depthWriteEnable = m_depthWriteEnable ? VK_TRUE : VK_FALSE,
            .depthCompareOp = VK_COMPARE_OP_LESS,
        };

        VkFormat colorFormat = static_cast<VkFormat>(m_colorFormat);
        VkPipelineRenderingCreateInfo renderingCI{
            .sType = VK_STRUCTURE_TYPE_PIPELINE_RENDERING_CREATE_INFO,
            .colorAttachmentCount = 1,
            .pColorAttachmentFormats = &colorFormat,
            .depthAttachmentFormat = m_depthTestEnable ? static_cast<VkFormat>(m_depthFormat) : VK_FORMAT_UNDEFINED
        };

        VkPipelineColorBlendAttachmentState blendAttachment{
            .colorWriteMask = 0xF
        };
        VkPipelineColorBlendStateCreateInfo colorBlendState{
            .sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO,
            .attachmentCount = 1,
            .pAttachments = &blendAttachment
        };
        VkPipelineRasterizationStateCreateInfo rasterizationState{
            .sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO,
            .lineWidth = 1.0f
        };
        VkPipelineMultisampleStateCreateInfo multisampleState{
            .sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO,
            .rasterizationSamples = VK_SAMPLE_COUNT_1_BIT
        };

        // Update shader stage entry point names to ensure they point to valid strings
        for (size_t i = 0; i < m_shaderStages.size(); ++i) {
            m_shaderStages[i].pName = m_shaderStageEntryPoints[i].c_str();
        }

        VkGraphicsPipelineCreateInfo pipelineCI{
            .sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO,
            .pNext = &renderingCI,
            .stageCount = 2,
            .pStages = m_shaderStages.data(),
            .pVertexInputState = &vertexInputState,
            .pInputAssemblyState = &inputAssemblyState,
            .pViewportState = &viewportState,
            .pRasterizationState = &rasterizationState,
            .pMultisampleState = &multisampleState,
            .pDepthStencilState = &depthStencilState,
            .pColorBlendState = &colorBlendState,
            .pDynamicState = &dynamicState,
            .layout = pipelineLayout
        };

        VkPipeline pipeline = VK_NULL_HANDLE;
        CheckVk(vkCreateGraphicsPipelines(m_device.GetRaw(), VK_NULL_HANDLE, 1, &pipelineCI, nullptr, &pipeline));
        
        GraphicsPipeline graphicsPipeline;
        PipelineLayout pipelineLayoutWrapper;
        pipelineLayoutWrapper.m_pipelineLayout = pipelineLayout;
        graphicsPipeline.m_pipelineLayout = pipelineLayoutWrapper;
        graphicsPipeline.m_pipeline = pipeline;
        graphicsPipeline.m_device = &m_device;
        return graphicsPipeline;
    }

    void GraphicsPipeline::Destroy()
    {
        if (m_pipeline != VK_NULL_HANDLE) {
            vkDestroyPipeline(m_device->GetRaw(), m_pipeline, nullptr);
            m_pipeline = VK_NULL_HANDLE;
        }
        if (m_pipelineLayout.GetRaw() != VK_NULL_HANDLE) {
            vkDestroyPipelineLayout(m_device->GetRaw(), m_pipelineLayout.GetRaw(), nullptr);
            m_pipelineLayout = PipelineLayout();
        }
    }
}