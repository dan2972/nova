#pragma once

#include <volk.h>
#include <span>
#include <utility>
#include <type_traits>

#include "nv_buffer.h"
#include "nv_helpers.h"
#include "nv_image.h"
#include "nv_synchronization.h"
#include "nv_shader.h"

namespace Nova {
    class Device;
    class GraphicsPipeline;
    class PipelineLayout;
    class DescriptorSet;
    enum class ShaderStageFlag : VkShaderStageFlags;

    enum class CommandPoolCreateFlag : VkCommandPoolCreateFlags {
        None = 0,
        Transient = VK_COMMAND_POOL_CREATE_TRANSIENT_BIT,
        ResetCommandBuffer = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT,
        Protected = VK_COMMAND_POOL_CREATE_PROTECTED_BIT
    };

    ENABLE_BITMASK_OPERATORS(CommandPoolCreateFlag, VkCommandPoolCreateFlags)

    enum class CommandBufferBeginFlag : VkCommandBufferUsageFlags {
        None = 0,
        OneTimeSubmit = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT,
        RenderPassContinue = VK_COMMAND_BUFFER_USAGE_RENDER_PASS_CONTINUE_BIT,
        SimultaneousUse = VK_COMMAND_BUFFER_USAGE_SIMULTANEOUS_USE_BIT
    };

    ENABLE_BITMASK_OPERATORS(CommandBufferBeginFlag, VkCommandBufferUsageFlags)

    enum class ImageLayout {
        Undefined = VK_IMAGE_LAYOUT_UNDEFINED,
        TransferDst = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
        ReadOnly = VK_IMAGE_LAYOUT_READ_ONLY_OPTIMAL,
        AttachmentOptimal = VK_IMAGE_LAYOUT_ATTACHMENT_OPTIMAL,
        PresentSrc = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR
    };

    enum class PipelineStageFlag : VkPipelineStageFlags2 {
        None = VK_PIPELINE_STAGE_2_NONE,
        Transfer = VK_PIPELINE_STAGE_2_TRANSFER_BIT,
        ColorAttachmentOutput = VK_PIPELINE_STAGE_2_COLOR_ATTACHMENT_OUTPUT_BIT,
        EarlyFragmentTests = VK_PIPELINE_STAGE_2_EARLY_FRAGMENT_TESTS_BIT,
        LateFragmentTests = VK_PIPELINE_STAGE_2_LATE_FRAGMENT_TESTS_BIT,
        FragmentShader = VK_PIPELINE_STAGE_2_FRAGMENT_SHADER_BIT
    };

    ENABLE_BITMASK_OPERATORS(PipelineStageFlag, VkPipelineStageFlags2)

    enum class AccessFlag : VkAccessFlags2 {
        None = VK_ACCESS_2_NONE,
        TransferWrite = VK_ACCESS_2_TRANSFER_WRITE_BIT,
        ShaderSampledRead = VK_ACCESS_2_SHADER_SAMPLED_READ_BIT,
        ColorAttachmentRead = VK_ACCESS_2_COLOR_ATTACHMENT_READ_BIT,
        ColorAttachmentWrite = VK_ACCESS_2_COLOR_ATTACHMENT_WRITE_BIT,
        DepthStencilAttachmentRead = VK_ACCESS_2_DEPTH_STENCIL_ATTACHMENT_READ_BIT,
        DepthStencilAttachmentWrite = VK_ACCESS_2_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT
    };

    ENABLE_BITMASK_OPERATORS(AccessFlag, VkAccessFlags2)

    enum class AttachmentLoadOp : std::underlying_type_t<VkAttachmentLoadOp> {
        Load = VK_ATTACHMENT_LOAD_OP_LOAD,
        Clear = VK_ATTACHMENT_LOAD_OP_CLEAR,
        DontCare = VK_ATTACHMENT_LOAD_OP_DONT_CARE
    };

    enum class AttachmentStoreOp : std::underlying_type_t<VkAttachmentStoreOp> {
        Store = VK_ATTACHMENT_STORE_OP_STORE,
        DontCare = VK_ATTACHMENT_STORE_OP_DONT_CARE
    };

    enum class IndexType : std::underlying_type_t<VkIndexType> {
        Uint16 = VK_INDEX_TYPE_UINT16,
        Uint32 = VK_INDEX_TYPE_UINT32
    };

    class SyncScope {
    public:
        SyncScope() = default;
        SyncScope(PipelineStageFlag stage, AccessFlag access)
            : stage(static_cast<VkPipelineStageFlags2>(stage)), access(static_cast<VkAccessFlags2>(access)) {}

        static SyncScope TransferWrite() {
            return SyncScope(PipelineStageFlag::Transfer, AccessFlag::TransferWrite);
        }

        static SyncScope FragmentShaderRead() {
            return SyncScope(PipelineStageFlag::FragmentShader, AccessFlag::ShaderSampledRead);
        }
    
    private:
        VkPipelineStageFlags2 stage = VK_PIPELINE_STAGE_2_NONE;
        VkAccessFlags2 access = VK_ACCESS_2_NONE;

        friend class CommandBuffer;
        friend class Device;
    };

    struct ImageSubresourceRange {
        ImageAspectFlag aspect = ImageAspectFlag::Color;
        uint32_t baseMipLevel = 0;
        uint32_t levelCount = 1;
        uint32_t baseArrayLayer = 0;
        uint32_t layerCount = 1;

        static constexpr ImageSubresourceRange Color(uint32_t mipLevels = 1) {
            return { .levelCount = mipLevels };
        }

        static constexpr ImageSubresourceRange DepthStencil(uint32_t mipLevels = 1) {
            return { .aspect = ImageAspectFlag::Depth | ImageAspectFlag::Stencil, .levelCount = mipLevels };
        }
    };

    struct ImageTransition {
        const Image* image = nullptr;
        ImageLayout oldLayout = ImageLayout::Undefined;
        ImageLayout newLayout = ImageLayout::Undefined;
        SyncScope src{};
        SyncScope dst{};
        ImageSubresourceRange range{};
    };

    struct BufferToImageCopyRegion {
        BufferSize bufferOffset = 0;
        ImageAspectFlag aspect = ImageAspectFlag::Color;
        uint32_t mipLevel = 0;
        uint32_t baseArrayLayer = 0;
        uint32_t layerCount = 1;
        Extent3D extent{1, 1, 1};
    };

    struct ClearValue {
        float color[4] = {0.0f, 0.0f, 0.0f, 1.0f};
    };

    enum class RenderAttachmentInfoType {
        Color,
        Depth,
        Stencil
    };

    struct RenderAttachmentInfo {
        RenderAttachmentInfoType type = RenderAttachmentInfoType::Color;
        const ImageView* imageView = nullptr;
        ImageLayout layout = ImageLayout::AttachmentOptimal;
        AttachmentLoadOp loadOp = AttachmentLoadOp::Clear;
        AttachmentStoreOp storeOp = AttachmentStoreOp::Store;
        ClearValue clearValue{0.0f, 0.0f, 0.0f, 1.0f};
    
        static constexpr RenderAttachmentInfo ColorAttachment(const ImageView& imageView, ClearValue clearValue = {0.0f, 0.0f, 0.0f, 1.0f}) {
            return RenderAttachmentInfo{
                .type = RenderAttachmentInfoType::Color,
                .imageView = &imageView,
                .layout = ImageLayout::AttachmentOptimal,
                .loadOp = AttachmentLoadOp::Clear,
                .storeOp = AttachmentStoreOp::Store,
                .clearValue = clearValue
            };
        }

        static constexpr RenderAttachmentInfo DepthAttachment(const ImageView& imageView, ClearValue clearValue = {1.0f, 0.0f, 0.0f, 0.0f}) {
            return RenderAttachmentInfo{
                .type = RenderAttachmentInfoType::Depth,
                .imageView = &imageView,
                .layout = ImageLayout::AttachmentOptimal,
                .loadOp = AttachmentLoadOp::Clear,
                .storeOp = AttachmentStoreOp::Store,
                .clearValue = clearValue
            };
        }
    };

    struct Viewport {
        float x = 0.0f;
        float y = 0.0f;
        float width = 1.0f;
        float height = 1.0f;
        float minDepth = 0.0f;
        float maxDepth = 1.0f;

        operator VkViewport() const {
            return VkViewport{x, y, width, height, minDepth, maxDepth};
        }
    };

    struct Rect2D {
        int32_t x = 0;
        int32_t y = 0;
        uint32_t width = 1;
        uint32_t height = 1;

        operator VkRect2D() const {
            return VkRect2D{{x, y}, {width, height}};
        }
    };

    struct CommandBufferSubmitInfo {
        std::vector<const CommandBuffer*> commandBuffers;
        std::vector<const Semaphore*> waitSemaphores;
        std::vector<const Semaphore*> signalSemaphores;
        PipelineStageFlag waitStageMask = PipelineStageFlag::None;
        const Fence* fence = nullptr;
    };

    class CommandPool {
    public:
        CommandPool() = default;
        ~CommandPool() { Destroy(); }

        CommandPool(const CommandPool&) = delete;
        CommandPool& operator=(const CommandPool&) = delete;

        CommandPool(CommandPool&& other) noexcept { *this = std::move(other); }
        CommandPool& operator=(CommandPool&& other) noexcept {
            if (this != &other) {
                Destroy();
                m_commandPool = other.m_commandPool;
                m_device = other.m_device;
                other.m_commandPool = VK_NULL_HANDLE;
                other.m_device = nullptr;
            }
            return *this;
        }

        VkCommandPool GetRaw() const { return m_commandPool; }
        void Destroy();
    private:
        VkCommandPool m_commandPool = VK_NULL_HANDLE;
        const Device* m_device = nullptr;

        friend class Device;
        friend class CommandBuffer;
    };

    class CommandBuffer {
    public:
        CommandBuffer() = default;
        ~CommandBuffer() { Destroy(); }

        CommandBuffer(const CommandBuffer&) = delete;
        CommandBuffer& operator=(const CommandBuffer&) = delete;

        CommandBuffer(CommandBuffer&& other) noexcept { *this = std::move(other); }
        CommandBuffer& operator=(CommandBuffer&& other) noexcept {
            if (this != &other) {
                Destroy();
                m_commandBuffer = other.m_commandBuffer;
                m_device = other.m_device;
                m_commandPool = other.m_commandPool;
                other.m_commandBuffer = VK_NULL_HANDLE;
                other.m_device = nullptr;
                other.m_commandPool = nullptr;
            }
            return *this;
        }

        VkCommandBuffer GetRaw() const { return m_commandBuffer; }
        void Destroy();

        CommandBuffer& Begin(CommandBufferBeginFlag flags = CommandBufferBeginFlag::OneTimeSubmit) {
            VkCommandBufferBeginInfo beginInfo{
                .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO,
                .flags = static_cast<VkCommandBufferUsageFlags>(flags)
            };
            CheckVk(vkBeginCommandBuffer(m_commandBuffer, &beginInfo));
            return *this;
        }

        CommandBuffer& Reset(bool releaseResources = false) {
            VkCommandBufferResetFlags flags = releaseResources ? VK_COMMAND_BUFFER_RESET_RELEASE_RESOURCES_BIT : 0;
            CheckVk(vkResetCommandBuffer(m_commandBuffer, flags));
            return *this;
        }

        CommandBuffer& End() {
            CheckVk(vkEndCommandBuffer(m_commandBuffer));
            return *this;
        }

        CommandBuffer& TransitionImage(const ImageTransition& transition);
        CommandBuffer& TransitionImages(const std::vector<ImageTransition>& imageTransitions);
        CommandBuffer& CopyBufferToImage(const Buffer& source, const Image& destination,
                               ImageLayout destinationLayout,
                               std::span<const BufferToImageCopyRegion> regions);
        CommandBuffer& BeginRendering(const std::vector<RenderAttachmentInfo>& attachmentInfos, const Extent2D& renderArea);
        CommandBuffer& SetViewport(const Viewport& viewport);
        CommandBuffer& SetScissor(const Rect2D& scissor);
        CommandBuffer& BindGraphicsPipeline(const GraphicsPipeline& pipeline);
        CommandBuffer& BindDescriptorSet(const PipelineLayout& pipelineLayout, const DescriptorSet& descriptorSet, uint32_t setIndex = 0);
        CommandBuffer& BindVertexBuffer(const Buffer& vertexBuffer, BufferSize offset = 0);
        CommandBuffer& BindIndexBuffer(const Buffer& indexBuffer, BufferSize offset = 0, IndexType indexType = IndexType::Uint32);
        CommandBuffer& PushConstants(const PipelineLayout& pipelineLayout, ShaderStageFlag stageFlags, uint32_t offset, uint32_t size, const void* pValues);
        CommandBuffer& DrawIndexed(uint32_t indexCount, uint32_t instanceCount = 1, uint32_t firstIndex = 0, int32_t vertexOffset = 0, uint32_t firstInstance = 0);
        CommandBuffer& EndRendering();

    private:
        VkCommandBuffer m_commandBuffer = VK_NULL_HANDLE;
        const Device* m_device = nullptr;
        const CommandPool* m_commandPool = nullptr;

        friend class Device;
    };
}
