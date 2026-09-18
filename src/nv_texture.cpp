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
        
        commandBuffer.TransitionImage(img, {
            .oldLayout = ImageLayout::Undefined,
            .newLayout = ImageLayout::TransferDst,
            .dst = SyncScope::TransferWrite(),
            .range = ImageSubresourceRange::Color(ktxTexture->numLevels),
        });

        std::vector<BufferToImageCopyRegion> copyRegions{};
        copyRegions.reserve(ktxTexture->numLevels);
        for (auto j = 0; j < ktxTexture->numLevels; j++) {
            ktx_size_t mipOffset{0};
            KTX_error_code ret = ktxTexture_GetImageOffset(ktxTexture, j, 0, 0, &mipOffset);
            if (ret != KTX_SUCCESS) {
                LogError("Failed to obtain mip {} offset for texture: {}", j, filePath);
                ktxTexture_Destroy(ktxTexture);
                return Texture();
            }
            copyRegions.push_back({
                .bufferOffset = mipOffset,
                .mipLevel = static_cast<uint32_t>(j),
                .extent = { .width = ktxTexture->baseWidth >> j, .height = ktxTexture->baseHeight >> j, .depth = 1 },
            });
        }
        commandBuffer.CopyBufferToImage(stagingBuffer, img, ImageLayout::TransferDst, copyRegions);

        commandBuffer.TransitionImage(img, {
            .oldLayout = ImageLayout::TransferDst,
            .newLayout = ImageLayout::ReadOnly,
            .src = SyncScope::TransferWrite(),
            .dst = SyncScope::FragmentShaderRead(),
            .range = ImageSubresourceRange::Color(ktxTexture->numLevels),
        });

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
