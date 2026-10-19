#include "nv_commandBuffer.h"

#include "nv_device.h"
#include <vector>
#include <algorithm>

namespace Nova {
    void CommandBuffer::Destroy()
    {
        if (m_commandBuffer != VK_NULL_HANDLE && m_commandPool != nullptr) {
            vkFreeCommandBuffers(m_device->GetRaw(), m_commandPool->GetRaw(), 1, &m_commandBuffer);
            m_commandBuffer = VK_NULL_HANDLE;
        }
    }

    CommandBuffer& CommandBuffer::TransitionImage(const ImageTransition& transition)
    {
        const VkImageMemoryBarrier2 barrier{
            .sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER_2,
            .srcStageMask = transition.src.stage,
            .srcAccessMask = transition.src.access,
            .dstStageMask = transition.dst.stage,
            .dstAccessMask = transition.dst.access,
            .oldLayout = static_cast<VkImageLayout>(transition.oldLayout),
            .newLayout = static_cast<VkImageLayout>(transition.newLayout),
            .image = transition.image->GetRaw(),
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
        return *this;
    }

    CommandBuffer& CommandBuffer::TransitionImages(const std::vector<ImageTransition>& imageTransitions)
    {
        std::vector<VkImageMemoryBarrier2> barriers;
        barriers.reserve(imageTransitions.size());
        for (const ImageTransition& transition : imageTransitions) {
            barriers.push_back({
                .sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER_2,
                .srcStageMask = transition.src.stage,
                .srcAccessMask = transition.src.access,
                .dstStageMask = transition.dst.stage,
                .dstAccessMask = transition.dst.access,
                .oldLayout = static_cast<VkImageLayout>(transition.oldLayout),
                .newLayout = static_cast<VkImageLayout>(transition.newLayout),
                .image = transition.image->GetRaw(),
                .subresourceRange = {
                    .aspectMask = static_cast<VkImageAspectFlags>(transition.range.aspect),
                    .baseMipLevel = transition.range.baseMipLevel,
                    .levelCount = transition.range.levelCount,
                    .baseArrayLayer = transition.range.baseArrayLayer,
                    .layerCount = transition.range.layerCount,
                },
            });
        }

        const VkDependencyInfo dependency{
            .sType = VK_STRUCTURE_TYPE_DEPENDENCY_INFO,
            .imageMemoryBarrierCount = static_cast<uint32_t>(barriers.size()),
            .pImageMemoryBarriers = barriers.data(),
        };
        vkCmdPipelineBarrier2(m_commandBuffer, &dependency);
        return *this;
    }

    CommandBuffer& CommandBuffer::CopyBufferToImage(const Buffer& source, const Image& destination,
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
        return *this;
    }

    CommandBuffer& CommandBuffer::BeginRendering(const std::vector<RenderAttachmentInfo> &attachmentInfos, const Extent2D &renderArea)
    {
        std::vector<VkRenderingAttachmentInfo> colorAttachmentInfos;
        std::vector<VkRenderingAttachmentInfo> depthStencilAttachmentInfos;
        std::vector<VkRenderingAttachmentInfo> stencilAttachmentInfos;
        for (const RenderAttachmentInfo& attachmentInfo : attachmentInfos) {
            VkRenderingAttachmentInfo vkAttachmentInfo{
                .sType = VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO,
                .imageView = attachmentInfo.imageView->GetRaw(),
                .imageLayout = static_cast<VkImageLayout>(attachmentInfo.layout),
                .loadOp = static_cast<VkAttachmentLoadOp>(attachmentInfo.loadOp),
                .storeOp = static_cast<VkAttachmentStoreOp>(attachmentInfo.storeOp),
                .clearValue = attachmentInfo.type == RenderAttachmentInfoType::Color ? VkClearValue{
                    .color = {
                        attachmentInfo.clearValue.color[0],
                        attachmentInfo.clearValue.color[1],
                        attachmentInfo.clearValue.color[2],
                        attachmentInfo.clearValue.color[3]
                    }
                } : VkClearValue{
                    .depthStencil = {
                        attachmentInfo.clearValue.color[0],
                        static_cast<uint32_t>(attachmentInfo.clearValue.color[1])
                    }
                }
            };

            if (attachmentInfo.type == RenderAttachmentInfoType::Color) {
                colorAttachmentInfos.push_back(vkAttachmentInfo);
            } else if (attachmentInfo.type == RenderAttachmentInfoType::Depth) {
                depthStencilAttachmentInfos.push_back(vkAttachmentInfo);
            } else if (attachmentInfo.type == RenderAttachmentInfoType::Stencil) {
                stencilAttachmentInfos.push_back(vkAttachmentInfo);
            }
        }

        VkRenderingInfo renderingInfo{
            .sType = VK_STRUCTURE_TYPE_RENDERING_INFO,
            .renderArea = {
                .offset = {0, 0},
                .extent = renderArea
            },
            .layerCount = 1,
            .colorAttachmentCount = static_cast<uint32_t>(colorAttachmentInfos.size()),
            .pColorAttachments = colorAttachmentInfos.data(),
            .pDepthAttachment = depthStencilAttachmentInfos.empty() ? nullptr : &depthStencilAttachmentInfos[0],
            .pStencilAttachment = stencilAttachmentInfos.empty() ? nullptr : &stencilAttachmentInfos[0],
        };

        vkCmdBeginRendering(m_commandBuffer, &renderingInfo);

        return *this;
    }

    CommandBuffer& CommandBuffer::SetViewport(const Viewport &viewport)
    {
        vkCmdSetViewport(m_commandBuffer, 0, 1, &static_cast<const VkViewport&>(viewport));
        return *this;
    }

    CommandBuffer& CommandBuffer::SetScissor(const Rect2D &scissor)
    {
        vkCmdSetScissor(m_commandBuffer, 0, 1, &static_cast<const VkRect2D&>(scissor));
        return *this;
    }

    CommandBuffer& CommandBuffer::BindGraphicsPipeline(const GraphicsPipeline &pipeline)
    {
        vkCmdBindPipeline(m_commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline.GetRaw());
        return *this;
    }

    CommandBuffer& CommandBuffer::BindDescriptorSet(const PipelineLayout &pipelineLayout, const DescriptorSet &descriptorSet, uint32_t setIndex)
    {
        vkCmdBindDescriptorSets(m_commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipelineLayout.GetRaw(), setIndex, 1, &descriptorSet.GetRaw(), 0, nullptr);
        return *this;
    }

    CommandBuffer& CommandBuffer::BindVertexBuffer(const Buffer &vertexBuffer, BufferSize offset)
    {
        VkBuffer vkBuffer = vertexBuffer.GetRaw();
        vkCmdBindVertexBuffers(m_commandBuffer, 0, 1, &vkBuffer, &offset);
        return *this;
    }

    CommandBuffer& CommandBuffer::BindIndexBuffer(const Buffer &indexBuffer, BufferSize offset, IndexType indexType)
    {
        vkCmdBindIndexBuffer(m_commandBuffer, indexBuffer.GetRaw(), offset, static_cast<VkIndexType>(indexType));
        return *this;
    }

    CommandBuffer& CommandBuffer::PushConstants(const PipelineLayout &pipelineLayout, ShaderStageFlag stageFlags, uint32_t offset, uint32_t size, const void *pValues)
    {
        vkCmdPushConstants(m_commandBuffer, pipelineLayout.GetRaw(), static_cast<VkShaderStageFlags>(stageFlags), offset, size, pValues);
        return *this;
    }

    CommandBuffer& CommandBuffer::DrawIndexed(uint32_t indexCount, uint32_t instanceCount, uint32_t firstIndex, int32_t vertexOffset, uint32_t firstInstance)
    {
        vkCmdDrawIndexed(m_commandBuffer, indexCount, instanceCount, firstIndex, vertexOffset, firstInstance);
        return *this;
    }

    CommandBuffer& CommandBuffer::EndRendering()
    {
        vkCmdEndRendering(m_commandBuffer);
        return *this;
    }

    void CommandPool::Destroy()
    {
        if (m_commandPool != VK_NULL_HANDLE) {
            vkDestroyCommandPool(m_device->GetRaw(), m_commandPool, nullptr);
            m_commandPool = VK_NULL_HANDLE;
        }
    }
}
