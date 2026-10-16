#pragma once

#include <volk.h>
#include <type_traits>
#include <vector>
#include <string>

#include "nv_helpers.h"
#include "nv_descriptors.h"
#include "nv_shader.h"
#include "nv_image.h"

namespace Nova {
    class Device;

    enum class VertexFormat : std::underlying_type_t<VkFormat> {
        None = VK_FORMAT_UNDEFINED,
        Float = VK_FORMAT_R32_SFLOAT,
        Float2 = VK_FORMAT_R32G32_SFLOAT,
        Float3 = VK_FORMAT_R32G32B32_SFLOAT,
        Float4 = VK_FORMAT_R32G32B32A32_SFLOAT,
        Int = VK_FORMAT_R32_SINT,
        Int2 = VK_FORMAT_R32G32_SINT,
        Int3 = VK_FORMAT_R32G32B32_SINT,
        Int4 = VK_FORMAT_R32G32B32A32_SINT,
        UInt = VK_FORMAT_R32_UINT,
        UInt2 = VK_FORMAT_R32G32_UINT,
        UInt3 = VK_FORMAT_R32G32B32_UINT,
        UInt4 = VK_FORMAT_R32G32B32A32_UINT,
        UByte4 = VK_FORMAT_R8G8B8A8_UINT,
        UByte4Norm = VK_FORMAT_R8G8B8A8_UNORM,
    };

    enum class ShaderStageFlag : VkShaderStageFlags {
        None = 0,
        Vertex = VK_SHADER_STAGE_VERTEX_BIT,
        Fragment = VK_SHADER_STAGE_FRAGMENT_BIT,
        Compute = VK_SHADER_STAGE_COMPUTE_BIT,
        Geometry = VK_SHADER_STAGE_GEOMETRY_BIT
    };

    ENABLE_BITMASK_OPERATORS(ShaderStageFlag, VkShaderStageFlags)

    struct VertexAttributeDescription {
        VertexFormat format;
        uint32_t offset;
    };

    struct VertexBindingLayout {
        uint32_t binding;
        uint32_t stride;
        std::vector<VertexAttributeDescription> attributes;
    };

    struct PushConstantRange {
        ShaderStageFlag stageFlags;
        uint32_t offset;
        uint32_t size;
    };

    class PipelineLayout {
    public:
        PipelineLayout() = default;

        const VkPipelineLayout& GetRaw() const { return m_pipelineLayout; }
    private:
        VkPipelineLayout m_pipelineLayout = VK_NULL_HANDLE;

        friend class GraphicsPipelineBuilder;
        friend class GraphicsPipeline;
    };

    class GraphicsPipeline {
    public:
        GraphicsPipeline() = default;
        ~GraphicsPipeline() { Destroy(); }

        const PipelineLayout& GetPipelineLayout() const { return m_pipelineLayout; }
        VkPipeline GetRaw() const { return m_pipeline; }

        void Destroy();
    private:
        const Device* m_device = nullptr;
        PipelineLayout m_pipelineLayout;
        VkPipeline m_pipeline = VK_NULL_HANDLE;
        friend class GraphicsPipelineBuilder;
    };

    class GraphicsPipelineBuilder {
    public:
        GraphicsPipelineBuilder(const Device& device) : m_device(device) {};
        ~GraphicsPipelineBuilder() = default;

        GraphicsPipelineBuilder& AddPushConstantRange(const PushConstantRange& pushConstantRange);
        GraphicsPipelineBuilder& AddDescriptorSetLayout(const DescriptorSetLayout& descriptorSetLayout);
        GraphicsPipelineBuilder& AddVertexBindingLayout(const VertexBindingLayout& vertexBindingLayout);
        GraphicsPipelineBuilder& AddShaderStage(ShaderStageFlag stage, const ShaderModule& shaderModule, const std::string& entryPoint = "main");
        GraphicsPipelineBuilder& EnableDepthStencil(bool depthTestEnable, bool depthWriteEnable, ImageFormat depthFormat);
        GraphicsPipelineBuilder& SetColorFormat(ImageFormat colorFormat) { m_colorFormat = colorFormat; return *this; }

        GraphicsPipeline Build();

    private:
        const Device& m_device;
        std::vector<VkPushConstantRange> m_pushConstantRanges;
        std::vector<VkDescriptorSetLayout> m_descriptorSetLayouts;
        
        std::vector<VkVertexInputBindingDescription> m_vertexInputBindingDescriptions;
        std::vector<VkVertexInputAttributeDescription> m_vertexInputAttributeDescriptions;

        std::vector<VkPipelineShaderStageCreateInfo> m_shaderStages;

        std::vector<std::string> m_shaderStageEntryPoints;

        bool m_depthTestEnable = false;
        bool m_depthWriteEnable = false;
        ImageFormat m_depthFormat;

        ImageFormat m_colorFormat = ImageFormat::B8G8R8A8Unorm;
    };
}