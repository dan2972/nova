#pragma once

#include <volk.h>
#include "nv_allocator.h"
#include "nv_helpers.h"

namespace nova {
    class Device;
    class Instance;
    class Texture;
    class ImageView;

    enum class ImageFormat {
        R8G8B8A8Unorm = VK_FORMAT_R8G8B8A8_UNORM,
        B8G8R8A8Unorm = VK_FORMAT_B8G8R8A8_UNORM,
        B8G8R8A8Srgb = VK_FORMAT_B8G8R8A8_SRGB,
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

    ENABLE_BITMASK_OPERATORS(ImageAspectFlag, VkImageAspectFlags)

    struct ImageCreateInfo {
        ImageFormat format = ImageFormat::R8G8B8A8Unorm;
        Extent3D extent{1, 1, 1};
        uint32_t mipLevels = 1;
        ImageUsageFlag usage = ImageUsageFlag::ColorAttachment;
        AllocationFlag allocationFlags = AllocationFlag::None;
    };

    class Image {
    public:
        Image() = default;
        Image(const Device& device, const ImageCreateInfo& createInfo);
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
        Extent3D m_extent{1, 1, 1};
        ImageUsageFlag m_usageFlags = ImageUsageFlag::None;
        uint32_t m_mipLevels = 1;

        friend class ImageBuilder;
        friend class Texture;
        friend class Swapchain;
    };

    class ImageView {
    public:
        ImageView() = default;
        ~ImageView() { Destroy(); }

        ImageView(const ImageView&) = delete;
        ImageView& operator=(const ImageView&) = delete;

        ImageView(ImageView&& other) noexcept { *this = std::move(other); }
        ImageView& operator=(ImageView&& other) noexcept {
            if (this != &other) {
                Destroy();
                m_imageView = other.m_imageView;
                m_device = other.m_device;
                m_ownsImageView = other.m_ownsImageView;
                other.m_imageView = VK_NULL_HANDLE;
                other.m_device = nullptr;
                other.m_ownsImageView = false;
            }
            return *this;
        }

        void Destroy();
        VkImageView GetRaw() const { return m_imageView; }
    private:
        VkImageView m_imageView = VK_NULL_HANDLE;
        const Device* m_device = nullptr;
        bool m_ownsImageView = true; // Swapchain image views are owned by the swapchain, so we don't want to destroy them when the ImageView is destroyed.

        friend class Image;
        friend class Swapchain;
    };
}
