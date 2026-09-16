#pragma once

#include <volk.h>
#include "nv_allocator.h"

namespace Nova {
    class Device;
    class Instance;

    enum class ImageFormat {
        R8G8B8A8Unorm = VK_FORMAT_R8G8B8A8_UNORM,
        B8G8R8A8Unorm = VK_FORMAT_B8G8R8A8_UNORM,
        R16G16B16A16Sfloat = VK_FORMAT_R16G16B16A16_SFLOAT,
        A2B10G10R10UnormPack32 = VK_FORMAT_A2B10G10R10_UNORM_PACK32,

        R8G8B8A8Snorm = VK_FORMAT_R8G8B8A8_SNORM,
        B8G8R8A8Snorm = VK_FORMAT_B8G8R8A8_SNORM,
        R16G16B16A16Snorm = VK_FORMAT_R16G16B16A16_SNORM,

        // Depth formats
        D16Unorm = VK_FORMAT_D16_UNORM,
        D32Sfloat = VK_FORMAT_D32_SFLOAT,
        D24UnormS8Uint = VK_FORMAT_D24_UNORM_S8_UINT,
        D32SfloatS8Uint = VK_FORMAT_D32_SFLOAT_S8_UINT
    };

    enum class ImageUsageFlag : VkImageUsageFlags {
        None = 0,
        TransferSrc = VK_IMAGE_USAGE_TRANSFER_SRC_BIT,
        TransferDst = VK_IMAGE_USAGE_TRANSFER_DST_BIT,
        Sampled = VK_IMAGE_USAGE_SAMPLED_BIT,
        Storage = VK_IMAGE_USAGE_STORAGE_BIT,
        ColorAttachment = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT,
        DepthStencilAttachment = VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT,
        TransientAttachment = VK_IMAGE_USAGE_TRANSIENT_ATTACHMENT_BIT,
        InputAttachment = VK_IMAGE_USAGE_INPUT_ATTACHMENT_BIT
    };

    constexpr ImageUsageFlag operator|(ImageUsageFlag a, ImageUsageFlag b) {
        return static_cast<ImageUsageFlag>(static_cast<VkImageUsageFlags>(a) | static_cast<VkImageUsageFlags>(b));
    }

    constexpr ImageUsageFlag operator&(ImageUsageFlag a, ImageUsageFlag b) {
        return static_cast<ImageUsageFlag>(static_cast<VkImageUsageFlags>(a) & static_cast<VkImageUsageFlags>(b));
    }

    constexpr ImageUsageFlag operator~(ImageUsageFlag a) {
        return static_cast<ImageUsageFlag>(~static_cast<VkImageUsageFlags>(a));
    }

    class Image {
    public:
        Image() = default;
        ~Image() { Destroy(); }

        void Destroy() {
            if (m_imageView != VK_NULL_HANDLE) {
                vkDestroyImageView(m_device, m_imageView, nullptr);
                m_imageView = VK_NULL_HANDLE;
            }
            m_allocation.Destroy(m_device);
        }

        uint32_t GetWidth() const { return m_extent.width; }
        uint32_t GetHeight() const { return m_extent.height; }
        uint32_t GetDepth() const { return m_extent.depth; }
        ImageFormat GetFormat() const { return m_format; }

    private:
        ImageAllocation m_allocation;
        VkDevice m_device = VK_NULL_HANDLE;
        VkImageView m_imageView = VK_NULL_HANDLE;
        ImageFormat m_format = ImageFormat::R8G8B8A8Unorm;
        VkExtent3D m_extent{1, 1, 1};

        friend class ImageBuilder;
    };

    class ImageBuilder {
    public:
        ImageBuilder(const Device* device) : m_device(device) {
            m_createInfo.extent = {1, 1, 1};
        }

        ImageBuilder& SetFormat(ImageFormat format) {
            m_createInfo.format = static_cast<VkFormat>(format);
            return *this;
        }

        ImageBuilder& SetExtent(uint32_t width, uint32_t height, uint32_t depth = 1) {
            m_createInfo.extent.width = width;
            m_createInfo.extent.height = height;
            m_createInfo.extent.depth = depth;
            return *this;
        }

        ImageBuilder& SetUsage(ImageUsageFlag usage) {
            m_createInfo.usage = static_cast<VkImageUsageFlags>(usage);
            return *this;
        }

        Image build(const Instance& instance);
    private:
        VkImageCreateInfo m_createInfo{
            .sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO,
            .imageType = VK_IMAGE_TYPE_2D,
            .format = VK_FORMAT_B8G8R8A8_SRGB,
            .mipLevels = 1,
            .arrayLayers = 1,
            .samples = VK_SAMPLE_COUNT_1_BIT,
            .tiling = VK_IMAGE_TILING_OPTIMAL,
            .usage = VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT,
            .initialLayout = VK_IMAGE_LAYOUT_UNDEFINED,
        };
        const Device* m_device = nullptr;
    };
}
