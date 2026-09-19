#include "nv_commandBuffer.h"

#include "nv_device.h"
#include <vector>

namespace Nova {
    void CommandBuffer::Destroy()
    {
        if (m_commandBuffer != VK_NULL_HANDLE && m_commandPool != nullptr) {
            vkFreeCommandBuffers(m_device->GetRaw(), m_commandPool->GetRaw(), 1, &m_commandBuffer);
            m_commandBuffer = VK_NULL_HANDLE;
        }
    }

    void CommandBuffer::TransitionImage(const Image& image, const ImageTransition& transition)
    {
        const VkImageMemoryBarrier2 barrier{
            .sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER_2,
            .srcStageMask = transition.src.stage,
            .srcAccessMask = transition.src.access,
            .dstStageMask = transition.dst.stage,
            .dstAccessMask = transition.dst.access,
            .oldLayout = static_cast<VkImageLayout>(transition.oldLayout),
            .newLayout = static_cast<VkImageLayout>(transition.newLayout),
            .image = image.GetRaw(),
            .subresourceRange = {
                .aspectMask = static_cast<VkImageAspectFlags>(transition.range.aspect),
                .baseMipLevel = transition.range.baseMipLevel,
                .levelCount = transition.range.levelCount,
                .baseArrayLayer = transition.range.baseArrayLayer,
                .layerCount = transition.range.layerCount,
            },
        };
        const VkDependencyInfo dependency{
            .sType = VK_STRUCTURE_TYPE_DEPENDENCY_INFO,
            .imageMemoryBarrierCount = 1,
            .pImageMemoryBarriers = &barrier,
        };
        vkCmdPipelineBarrier2(m_commandBuffer, &dependency);
    }

    void CommandBuffer::CopyBufferToImage(const Buffer& source, const Image& destination,
                                           ImageLayout destinationLayout,
                                           std::span<const BufferToImageCopyRegion> regions)
    {
        std::vector<VkBufferImageCopy> vkRegions;
        vkRegions.reserve(regions.size());
        for (const BufferToImageCopyRegion& region : regions) {
            vkRegions.push_back({
                .bufferOffset = region.bufferOffset,
                .imageSubresource = {
                    .aspectMask = static_cast<VkImageAspectFlags>(region.aspect),
                    .mipLevel = region.mipLevel,
                    .baseArrayLayer = region.baseArrayLayer,
                    .layerCount = region.layerCount,
                },
                .imageExtent = region.extent,
            });
        }

        vkCmdCopyBufferToImage(m_commandBuffer, source.GetRaw(), destination.GetRaw(),
                               static_cast<VkImageLayout>(destinationLayout),
                               static_cast<uint32_t>(vkRegions.size()), vkRegions.data());
    }

    void CommandPool::Destroy()
    {
        if (m_commandPool != VK_NULL_HANDLE) {
            vkDestroyCommandPool(m_device->GetRaw(), m_commandPool, nullptr);
            m_commandPool = VK_NULL_HANDLE;
        }
    }
}
