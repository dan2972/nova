#pragma once

#include <volk.h>
#include "nv_allocator.h"
#include "nv_helpers.h"

namespace Nova {
    class Device;
    class Instance;
    class Texture;
    class ImageView;

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

    ENABLE_BITMASK_OPERATORS(ImageUsageFlag, VkImageUsageFlags)

    enum class ImageAspectFlag : VkImageAspectFlags {
        None = 0,
        Color = VK_IMAGE_ASPECT_COLOR_BIT,
        Depth = VK_IMAGE_ASPECT_DEPTH_BIT,
        Stencil = VK_IMAGE_ASPECT_STENCIL_BIT,
        Metadata = VK_IMAGE_ASPECT_METADATA_BIT
    };

    class Image {
    public:
        Image() = default;
        ~Image() { Destroy(); }

        Image(const Image&) = delete;
        Image& operator=(const Image&) = delete;

        Image(Image&& other) noexcept { *this = std::move(other); }
        Image& operator=(Image&& other) noexcept {
            if (this != &other) {
                Destroy();
                m_allocation = std::move(other.m_allocation);
                m_device = other.m_device;
                m_format = other.m_format;
                m_extent = other.m_extent;
                m_usageFlags = other.m_usageFlags;
                m_mipLevels = other.m_mipLevels;
                other.m_device = nullptr;
            }
            return *this;
        }

        void Destroy() {
            m_allocation.Destroy();
        }

        uint32_t GetWidth() const { return m_extent.width; }
        uint32_t GetHeight() const { return m_extent.height; }
        uint32_t GetDepth() const { return m_extent.depth; }
        ImageFormat GetFormat() const { return m_format; }
        VkImage GetRaw() const { return m_allocation.image; }

        ImageView CreateView() const;
        ImageView CreateView(ImageAspectFlag aspectMask) const;

    private:
        ImageAllocation m_allocation;
        const Device* m_device = nullptr;
        ImageFormat m_format = ImageFormat::R8G8B8A8Unorm;
        VkExtent3D m_extent{1, 1, 1};
        VkImageUsageFlags m_usageFlags = 0;
        uint32_t m_mipLevels = 1;

        friend class ImageBuilder;
        friend class Texture;
    };

    class ImageView {
    public:
        ImageView() = default;
        ~ImageView() { Destroy(); }

        void Destroy() {
            if (m_imageView != VK_NULL_HANDLE) {
                vkDestroyImageView(m_device, m_imageView, nullptr);
                m_imageView = VK_NULL_HANDLE;
            }
        }
    private:
        VkImageView m_imageView = VK_NULL_HANDLE;
        VkDevice m_device = VK_NULL_HANDLE;

        friend class Image;
    };

    class ImageBuilder {
    public:
        ImageBuilder(const Device& device) : m_device(&device) {
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

        ImageBuilder&  SetMipLevels(uint32_t mipLevels) {
            m_createInfo.mipLevels = mipLevels;
            return *this;
        }

        Image build(const Instance& instance, AllocationFlag allocationFlags = AllocationFlag::None);
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
