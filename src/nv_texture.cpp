#include "nv_texture.h"

#include "nv_device.h"
#include "nv_helpers.h"
#include "nv_commandBuffer.h"

namespace Nova {
    Texture Texture::CreateFromFile(const std::string &filePath, Device* device, CommandPool* commandPool)
    {
        if (!device) {
            LogError("Device pointer is null when creating texture from file: {}", filePath);
            return Texture();
        }

        if (!commandPool) {
            LogError("CommandPool pointer is null when creating texture from file: {}", filePath);
            return Texture();
        }

        ktxTexture* ktxTexture = nullptr;
        ktxTexture_CreateFromNamedFile(filePath.c_str(), KTX_TEXTURE_CREATE_LOAD_IMAGE_DATA_BIT, &ktxTexture);

        if (!ktxTexture) {
            LogError("Failed to load texture from file: {}", filePath);
            return Texture();
        }

        Texture texture;

        ImageBuilder imageBuilder(device);
        Image img = imageBuilder
            .SetFormat(static_cast<ImageFormat>(ktxTexture_GetVkFormat(ktxTexture)))
            .SetExtent(ktxTexture->baseWidth, ktxTexture->baseHeight, ktxTexture->baseDepth)
            .SetUsage(ImageUsageFlag::Sampled | ImageUsageFlag::TransferDst)
            .SetMipLevels(ktxTexture->numLevels)
            .build(*device->GetInstance());
        
        // temp buffer for uploading texture data
        Buffer stagingBuffer = device->CreateBuffer(
            ktxTexture->dataSize, 
            BufferUsageFlag::TransferSrc, 
            AllocationFlag::HostAccessSequentialWrite | AllocationFlag::Mapped);

        memcpy(stagingBuffer.GetMappedData(), ktxTexture->pData, ktxTexture->dataSize);

        Fence fence = device->CreateFence();
        CommandBuffer commandBuffer = device->CreateCommandBuffer(*commandPool);

        commandBuffer.Begin();
        
        VkImageMemoryBarrier2 barrier{
            .sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER_2,
            .srcStageMask = VK_PIPELINE_STAGE_2_NONE,
            .srcAccessMask = VK_ACCESS_2_NONE,
            .dstStageMask = VK_PIPELINE_STAGE_2_TRANSFER_BIT,
            .dstAccessMask = VK_ACCESS_2_TRANSFER_WRITE_BIT,
            .oldLayout = VK_IMAGE_LAYOUT_UNDEFINED,
            .newLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
            .image = img.m_allocation.image,
            .subresourceRange = { .aspectMask = VK_IMAGE_ASPECT_COLOR_BIT, .levelCount = ktxTexture->numLevels, .layerCount = 1 }
        };
        VkDependencyInfo barrierTexInfo{
            .sType = VK_STRUCTURE_TYPE_DEPENDENCY_INFO,
            .imageMemoryBarrierCount = 1,
            .pImageMemoryBarriers = &barrier
        };

        vkCmdPipelineBarrier2(commandBuffer.GetRaw(), &barrierTexInfo);

        std::vector<VkBufferImageCopy> copyRegions{};
        for (auto j = 0; j < ktxTexture->numLevels; j++) {
            ktx_size_t mipOffset{0};
            KTX_error_code ret = ktxTexture_GetImageOffset(ktxTexture, j, 0, 0, &mipOffset);
            copyRegions.push_back({
                .bufferOffset = mipOffset,
                .imageSubresource{.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT, .mipLevel = (uint32_t)j, .layerCount = 1},
                .imageExtent{.width = ktxTexture->baseWidth >> j, .height = ktxTexture->baseHeight >> j, .depth = 1 },
            });
        }
        vkCmdCopyBufferToImage(commandBuffer.GetRaw(), stagingBuffer.GetRaw(), img.m_allocation.image, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, static_cast<uint32_t>(copyRegions.size()), copyRegions.data());

        VkImageMemoryBarrier2 barrierTexRead{
            .sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER_2,
            .srcStageMask = VK_PIPELINE_STAGE_TRANSFER_BIT,
            .srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT,
            .dstStageMask = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT,
            .dstAccessMask = VK_ACCESS_SHADER_READ_BIT,
            .oldLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
            .newLayout = VK_IMAGE_LAYOUT_READ_ONLY_OPTIMAL,
            .image = img.m_allocation.image,
            .subresourceRange = {.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT, .levelCount = ktxTexture->numLevels, .layerCount = 1 }
        };
        barrierTexInfo.pImageMemoryBarriers = &barrierTexRead;
        vkCmdPipelineBarrier2(commandBuffer.GetRaw(), &barrierTexInfo);

        commandBuffer.End();
        device->SubmitCommandBuffer(&commandBuffer, &fence);
        device->WaitForFence(&fence);

        VkSampler sampler{};
        VkSamplerCreateInfo samplerCI{
			.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO,
			.magFilter = VK_FILTER_LINEAR,
			.minFilter = VK_FILTER_LINEAR,
			.mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR,
			.anisotropyEnable = VK_TRUE,
			.maxAnisotropy = 8.0f,
			.maxLod = (float)ktxTexture->numLevels,
		};
		CheckVk(vkCreateSampler(device->GetRaw(), &samplerCI, nullptr, &sampler));
        ktxTexture_Destroy(ktxTexture);

        texture.m_image = std::move(img);
        texture.m_sampler = sampler;
        return texture;
    }
}