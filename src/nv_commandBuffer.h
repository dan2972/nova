#pragma once

#include <volk.h>
#include <span>
#include <utility>
#include <type_traits>
#include <vector>

#include "nv_buffer.h"
#include "nv_helpers.h"
#include "nv_image.h"
#include "nv_sampler.h"
#include "nv_synchronization.h"
#include "nv_shader.h"

namespace nova {
    class Device;
    class ComputePipeline;
    class GraphicsPipeline;
    class PipelineLayout;
    class DescriptorSet;

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
        General = VK_IMAGE_LAYOUT_GENERAL,
        TransferSrc = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
        TransferDst = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
        ReadOnly = VK_IMAGE_LAYOUT_READ_ONLY_OPTIMAL,
        AttachmentOptimal = VK_IMAGE_LAYOUT_ATTACHMENT_OPTIMAL,
        PresentSrc = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR
    };

    enum class PipelineStageFlag : VkPipelineStageFlags2 {
        None = VK_PIPELINE_STAGE_2_NONE,
        AllCommands = VK_PIPELINE_STAGE_2_ALL_COMMANDS_BIT,
        Transfer = VK_PIPELINE_STAGE_2_TRANSFER_BIT,
        ColorAttachmentOutput = VK_PIPELINE_STAGE_2_COLOR_ATTACHMENT_OUTPUT_BIT,
        EarlyFragmentTests = VK_PIPELINE_STAGE_2_EARLY_FRAGMENT_TESTS_BIT,
        LateFragmentTests = VK_PIPELINE_STAGE_2_LATE_FRAGMENT_TESTS_BIT,
        VertexShader = VK_PIPELINE_STAGE_2_VERTEX_SHADER_BIT,
        FragmentShader = VK_PIPELINE_STAGE_2_FRAGMENT_SHADER_BIT,
        ComputeShader = VK_PIPELINE_STAGE_2_COMPUTE_SHADER_BIT,
        VertexAttributeInput = VK_PIPELINE_STAGE_2_VERTEX_ATTRIBUTE_INPUT_BIT,
        IndexInput = VK_PIPELINE_STAGE_2_INDEX_INPUT_BIT,
        Host = VK_PIPELINE_STAGE_2_HOST_BIT
    };

    ENABLE_BITMASK_OPERATORS(PipelineStageFlag, VkPipelineStageFlags2)

    enum class AccessFlag : VkAccessFlags2 {
        None = VK_ACCESS_2_NONE,
        TransferRead = VK_ACCESS_2_TRANSFER_READ_BIT,
        TransferWrite = VK_ACCESS_2_TRANSFER_WRITE_BIT,
        UniformRead = VK_ACCESS_2_UNIFORM_READ_BIT,
        ShaderSampledRead = VK_ACCESS_2_SHADER_SAMPLED_READ_BIT,
        ShaderStorageRead = VK_ACCESS_2_SHADER_STORAGE_READ_BIT,
        ShaderStorageWrite = VK_ACCESS_2_SHADER_STORAGE_WRITE_BIT,
        VertexAttributeRead = VK_ACCESS_2_VERTEX_ATTRIBUTE_READ_BIT,
        IndexRead = VK_ACCESS_2_INDEX_READ_BIT,
        HostRead = VK_ACCESS_2_HOST_READ_BIT,
        HostWrite = VK_ACCESS_2_HOST_WRITE_BIT,
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
        constexpr SyncScope(PipelineStageFlag stage, AccessFlag access)
            : stage(static_cast<VkPipelineStageFlags2>(stage)), access(static_cast<VkAccessFlags2>(access)) {}

        static constexpr SyncScope TransferRead() {
            return SyncScope(PipelineStageFlag::Transfer, AccessFlag::TransferRead);
        }

        static constexpr SyncScope TransferWrite() {
            return SyncScope(PipelineStageFlag::Transfer, AccessFlag::TransferWrite);
        }

        static constexpr SyncScope ComputeShaderWrite() {
            return SyncScope(PipelineStageFlag::ComputeShader, AccessFlag::ShaderStorageWrite);
        }

        static constexpr SyncScope FragmentShaderRead() {
            return SyncScope(PipelineStageFlag::FragmentShader, AccessFlag::ShaderSampledRead);
        }

        constexpr bool HasWrites() const {
            constexpr auto writes = AccessFlag::TransferWrite | AccessFlag::ShaderStorageWrite |
                AccessFlag::ColorAttachmentWrite | AccessFlag::DepthStencilAttachmentWrite | AccessFlag::HostWrite;
            return (access & static_cast<VkAccessFlags2>(writes)) != 0;
        }

        constexpr bool HasAccess() const { return access != VK_ACCESS_2_NONE; }
        constexpr bool HasStage() const { return stage != VK_PIPELINE_STAGE_2_NONE; }

        constexpr SyncScope operator|(SyncScope other) const {
            return {static_cast<PipelineStageFlag>(stage | other.stage),
                    static_cast<AccessFlag>(access | other.access)};
        }
    
    private:
        VkPipelineStageFlags2 stage = VK_PIPELINE_STAGE_2_NONE;
        VkAccessFlags2 access = VK_ACCESS_2_NONE;

        friend class CommandBuffer;
        friend class Device;
        friend class BarrierBatch;
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

        static constexpr ImageTransition ToColorAttachment(const Image& image, ImageLayout oldLayout = ImageLayout::Undefined) {
            return {
                .image = &image,
                .oldLayout = oldLayout,
                .newLayout = ImageLayout::AttachmentOptimal,
                .src = {PipelineStageFlag::ColorAttachmentOutput, AccessFlag::None},
                .dst = {PipelineStageFlag::ColorAttachmentOutput, AccessFlag::ColorAttachmentRead | AccessFlag::ColorAttachmentWrite},
                .range = ImageSubresourceRange::Color()
            };
        }

        static constexpr ImageTransition ToDepthStencilAttachment(const Image& image, ImageLayout oldLayout = ImageLayout::Undefined) {
            return {
                .image = &image,
                .oldLayout = oldLayout,
                .newLayout = ImageLayout::AttachmentOptimal,
                .src = {PipelineStageFlag::LateFragmentTests, AccessFlag::DepthStencilAttachmentWrite},
                .dst = {PipelineStageFlag::EarlyFragmentTests, AccessFlag::DepthStencilAttachmentWrite},
                .range = ImageSubresourceRange::DepthStencil()
            };
        }

        static constexpr ImageTransition ToPresent(const Image& image, ImageLayout oldLayout, SyncScope src) {
            return {
                .image = &image,
                .oldLayout = oldLayout,
                .newLayout = ImageLayout::PresentSrc,
                .src = src,
                .range = ImageSubresourceRange::Color()
            };
        }

        static constexpr ImageTransition ToPresent(const Image& image, ImageLayout oldLayout = ImageLayout::AttachmentOptimal) {
            return ToPresent(image, oldLayout, oldLayout == ImageLayout::TransferDst
                ? SyncScope::TransferWrite()
                : SyncScope{PipelineStageFlag::ColorAttachmentOutput, AccessFlag::ColorAttachmentWrite});
        }
    };

    struct BufferBarrier {
        const Buffer* buffer = nullptr;
        SyncScope src{};
        SyncScope dst{};
        BufferSize offset = 0;
        BufferSize size = WholeBufferSize;
    };

    class BarrierBatch {
    public:
        void Add(const ImageTransition& transition);
        void Add(const BufferBarrier& barrier);
    private:
        static VkImageMemoryBarrier2 ToVkImageMemoryBarrier(const ImageTransition& transition);
        static VkBufferMemoryBarrier2 ToVkBufferMemoryBarrier(const BufferBarrier& barrier);
        std::vector<VkImageMemoryBarrier2> m_images;
        std::vector<VkBufferMemoryBarrier2> m_buffers;
        friend class CommandBuffer;
    };

    struct BufferToImageCopyRegion {
        BufferSize bufferOffset = 0;
        ImageAspectFlag aspect = ImageAspectFlag::Color;
        uint32_t mipLevel = 0;
        uint32_t baseArrayLayer = 0;
        uint32_t layerCount = 1;
        Extent3D extent{1, 1, 1};
        uint32_t bufferRowLength = 0;
        uint32_t bufferImageHeight = 0;
        int32_t imageOffsetX = 0;
        int32_t imageOffsetY = 0;
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
        CommandBuffer& PipelineBarrier(const BarrierBatch& batch);
        CommandBuffer& BarrierBuffer(const Buffer& buffer, SyncScope src, SyncScope dst,
                                     BufferSize offset = 0, BufferSize size = WholeBufferSize);
        CommandBuffer& CopyBufferToImage(const Buffer& source, const Image& destination,
                                         ImageLayout destinationLayout, std::span<const BufferToImageCopyRegion> regions);
        CommandBuffer& BlitImage(const Image& source, const Image& destination,
                                 ImageLayout sourceLayout = ImageLayout::TransferSrc,
                                 ImageLayout destinationLayout = ImageLayout::TransferDst,
                                 SamplerFilter filter = SamplerFilter::Nearest);
        CommandBuffer& BeginRendering(const std::vector<RenderAttachmentInfo>& attachmentInfos, const Extent2D& renderArea);
        CommandBuffer& SetViewport(const Viewport& viewport);
        CommandBuffer& SetScissor(const Rect2D& scissor);
        CommandBuffer& BindGraphicsPipeline(const GraphicsPipeline& pipeline);
        CommandBuffer& BindComputePipeline(const ComputePipeline& pipeline);
        CommandBuffer& BindDescriptorSet(const PipelineLayout& pipelineLayout, const DescriptorSet& descriptorSet, uint32_t setIndex = 0);
        CommandBuffer& BindVertexBuffer(const Buffer& vertexBuffer, BufferSize offset = 0);
        CommandBuffer& BindIndexBuffer(const Buffer& indexBuffer, BufferSize offset = 0, IndexType indexType = IndexType::Uint32);
        CommandBuffer& PushConstants(const PipelineLayout& pipelineLayout, ShaderStageFlag stageFlags, uint32_t offset, uint32_t size, const void* pValues);
        CommandBuffer& Dispatch(uint32_t groupCountX, uint32_t groupCountY = 1, uint32_t groupCountZ = 1);
        CommandBuffer& Draw(uint32_t vertexCount, uint32_t instanceCount = 1, uint32_t firstVertex = 0, uint32_t firstInstance = 0);
        CommandBuffer& DrawIndexed(uint32_t indexCount, uint32_t instanceCount = 1, uint32_t firstIndex = 0, int32_t vertexOffset = 0, uint32_t firstInstance = 0);
        CommandBuffer& EndRendering();

    private:
        VkCommandBuffer m_commandBuffer = VK_NULL_HANDLE;
        const Device* m_device = nullptr;
        const CommandPool* m_commandPool = nullptr;

        friend class Device;
    };

    struct CommandBufferSubmitInfo {
        std::vector<const CommandBuffer*> commandBuffers;
        std::vector<const Semaphore*> waitSemaphores;
        std::vector<const Semaphore*> signalSemaphores;
        PipelineStageFlag waitStageMask = PipelineStageFlag::AllCommands;
        const Fence* fence = nullptr;
    };
}
