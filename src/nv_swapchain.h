#pragma once

#include <volk.h>
#include <utility>
#include <vector>

#include "nv_helpers.h"
#include "nv_image.h"
#include "nv_surface.h"

namespace Nova {
    enum class ColorSpace {
        SrgbNonlinear = VK_COLOR_SPACE_SRGB_NONLINEAR_KHR,
        DisplayP3Nonlinear = VK_COLOR_SPACE_DISPLAY_P3_NONLINEAR_EXT,
        ExtendedSrgbLinear = VK_COLOR_SPACE_EXTENDED_SRGB_LINEAR_EXT,
        DciP3Nonlinear = VK_COLOR_SPACE_DCI_P3_NONLINEAR_EXT,
        Bt709Linear = VK_COLOR_SPACE_BT709_LINEAR_EXT,
        Bt709Nonlinear = VK_COLOR_SPACE_BT709_NONLINEAR_EXT,
        Bt2020Linear = VK_COLOR_SPACE_BT2020_LINEAR_EXT,
        Hdr10St2084 = VK_COLOR_SPACE_HDR10_ST2084_EXT,
        Dolbyvision = VK_COLOR_SPACE_DOLBYVISION_EXT,
        Hdr10Hlg = VK_COLOR_SPACE_HDR10_HLG_EXT
    };

    enum class SwapchainPresentMode {
        Immediate = VK_PRESENT_MODE_IMMEDIATE_KHR,
        Mailbox = VK_PRESENT_MODE_MAILBOX_KHR,
        Fifo = VK_PRESENT_MODE_FIFO_KHR,
        FifoRelaxed = VK_PRESENT_MODE_FIFO_RELAXED_KHR
    };

    class Device;

    class Swapchain {
    public:
        Swapchain() = default;
        ~Swapchain() { Destroy(); }

        Swapchain(const Swapchain&) = delete;
        Swapchain& operator=(const Swapchain&) = delete;

        Swapchain(Swapchain&& other) noexcept { *this = std::move(other); }

        Swapchain& operator=(Swapchain&& other) noexcept {
            if (this != &other) {
                Destroy();
                m_device = other.m_device;
                m_swapchain = other.m_swapchain;
                m_format = other.m_format;
                m_colorSpace = other.m_colorSpace;
                m_presentMode = other.m_presentMode;
                m_extent = other.m_extent;
                m_images = std::move(other.m_images);
                m_imageViews = std::move(other.m_imageViews);
                other.m_device = nullptr;
                other.m_swapchain = VK_NULL_HANDLE;
            }
            return *this;
        }

        Image AcquireNextImage(const Semaphore* semaphore = nullptr, const Fence* fence = nullptr, uint64_t timeout = UINT64_MAX) const;

        void Destroy();

        VkSwapchainKHR GetRaw() const { return m_swapchain; }
        ImageFormat GetFormat() const { return m_format; }
        ColorSpace GetColorSpace() const { return m_colorSpace; }
        SwapchainPresentMode GetPresentMode() const { return m_presentMode; }
        uint32_t GetWidth() const { return m_extent.width; }
        uint32_t GetHeight() const { return m_extent.height; }
        std::vector<VkImage> GetImages() const { return m_images; }
        std::vector<VkImageView> GetImageViews() const { return m_imageViews; }

    private:
        VkSwapchainKHR m_swapchain = VK_NULL_HANDLE;
        ImageFormat m_format;
        ColorSpace m_colorSpace;
        SwapchainPresentMode m_presentMode;
        Extent2D m_extent;
        std::vector<VkImage> m_images;
        std::vector<VkImageView> m_imageViews;
        const Device* m_device = nullptr;

        friend class SwapchainBuilder;
    };

    class SwapchainBuilder {
    public:
        SwapchainBuilder(const Device& device, const Surface& surface);

        SwapchainBuilder& SetDesiredFormat(ImageFormat format);

        SwapchainBuilder& SetDesiredColorSpace(ColorSpace colorSpace);

        SwapchainBuilder& SetDesiredPresentMode(SwapchainPresentMode presentMode);

        SwapchainBuilder& SetDesiredExtent(const Extent2D& extent);

        SwapchainBuilder& SetDesiredExtent(uint32_t width, uint32_t height) {
            SetDesiredExtent(Extent2D{width, height});
            return *this;
        }

        Swapchain build();
    private:
        VkSwapchainCreateInfoKHR m_createInfo{
            .sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR,
            .imageFormat = VK_FORMAT_B8G8R8A8_SRGB,
            .imageColorSpace = VK_COLOR_SPACE_SRGB_NONLINEAR_KHR,
            .imageArrayLayers = 1,
            .imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT,
            .preTransform = VK_SURFACE_TRANSFORM_IDENTITY_BIT_KHR,
            .compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR,
            .presentMode = VK_PRESENT_MODE_FIFO_KHR,
        };
        const Device* m_device;
        const Surface* m_surface;
    };
}
