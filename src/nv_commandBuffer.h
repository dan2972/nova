#pragma once

#include <volk.h>
#include <span>
#include <utility>
#include "nv_buffer.h"
#include "nv_helpers.h"
#include "nv_image.h"

namespace Nova {
    class Device;

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
    };

    struct SyncScope {
        VkPipelineStageFlags2 stage = VK_PIPELINE_STAGE_2_NONE;
        VkAccessFlags2 access = VK_ACCESS_2_NONE;

        static constexpr SyncScope TransferWrite() {
            return { VK_PIPELINE_STAGE_2_TRANSFER_BIT, VK_ACCESS_2_TRANSFER_WRITE_BIT };
        }

        static constexpr SyncScope FragmentShaderRead() {
            return { VK_PIPELINE_STAGE_2_FRAGMENT_SHADER_BIT, VK_ACCESS_2_SHADER_SAMPLED_READ_BIT };
        }
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
    };

    struct ImageTransition {
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
        VkExtent3D extent{1, 1, 1};
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
                other.m_device = VK_NULL_HANDLE;
            }
            return *this;
        }

        VkCommandPool GetRaw() const { return m_commandPool; }
        void Destroy() {
            if (m_commandPool != VK_NULL_HANDLE) {
                vkDestroyCommandPool(m_device, m_commandPool, nullptr);
                m_commandPool = VK_NULL_HANDLE;
            }
        }
    private:
        VkCommandPool m_commandPool = VK_NULL_HANDLE;
        VkDevice m_device = VK_NULL_HANDLE;

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
                other.m_device = VK_NULL_HANDLE;
                other.m_commandPool = VK_NULL_HANDLE;
            }
            return *this;
        }

        VkCommandBuffer GetRaw() const { return m_commandBuffer; }
        void Destroy() {
            if (m_commandBuffer != VK_NULL_HANDLE && m_commandPool != nullptr) {
                vkFreeCommandBuffers(m_device, m_commandPool->GetRaw(), 1, &m_commandBuffer);
                m_commandBuffer = VK_NULL_HANDLE;
            }
        }

        void Begin(CommandBufferBeginFlag flags = CommandBufferBeginFlag::None) {
            VkCommandBufferBeginInfo beginInfo{
                .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO,
                .flags = static_cast<VkCommandBufferUsageFlags>(flags)
            };
            CheckVk(vkBeginCommandBuffer(m_commandBuffer, &beginInfo));
        }

        void End() {
            CheckVk(vkEndCommandBuffer(m_commandBuffer));
        }

        void TransitionImage(const Image& image, const ImageTransition& transition);
        void CopyBufferToImage(const Buffer& source, const Image& destination,
                               ImageLayout destinationLayout,
                               std::span<const BufferToImageCopyRegion> regions);

    private:
        VkCommandBuffer m_commandBuffer = VK_NULL_HANDLE;
        VkDevice m_device = VK_NULL_HANDLE;
        const CommandPool* m_commandPool = nullptr;

        friend class Device;
    };
}
