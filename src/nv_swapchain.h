#pragma once

#include <volk.h>
#include <utility>
#include <vector>

#include "nv_device.h"
#include "nv_helpers.h"
#include "nv_image.h"

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
                other.m_device = VK_NULL_HANDLE;
                other.m_swapchain = VK_NULL_HANDLE;
            }
            return *this;
        }

        void Destroy() {
            if (m_device == VK_NULL_HANDLE) return;

            for (VkImageView imageView : m_imageViews) {
                if (imageView != VK_NULL_HANDLE) {
                    vkDestroyImageView(m_device, imageView, nullptr);
                }
            }
            m_imageViews.clear();
            m_images.clear();

            if (m_swapchain != VK_NULL_HANDLE) {
                vkDestroySwapchainKHR(m_device, m_swapchain, nullptr);
                m_swapchain = VK_NULL_HANDLE;
            }
            m_device = VK_NULL_HANDLE;
        }

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
        VkExtent2D m_extent;
        std::vector<VkImage> m_images;
        std::vector<VkImageView> m_imageViews;
        VkDevice m_device = VK_NULL_HANDLE;

        friend class SwapchainBuilder;
    };

    class SwapchainBuilder {
    public:
        SwapchainBuilder(Device* device) : m_device(device) {
            VkSurfaceCapabilitiesKHR surfaceCaps = device->GetSurfaceCapabilities();
            m_createInfo.surface = device->GetSurface();
            m_createInfo.minImageCount = surfaceCaps.minImageCount;
            m_createInfo.imageExtent = surfaceCaps.currentExtent;
        }

        SwapchainBuilder& SetFormat(ImageFormat format) {
            m_createInfo.imageFormat = static_cast<VkFormat>(format);
            return *this;
        }

        SwapchainBuilder& SetColorSpace(ColorSpace colorSpace) {
            m_createInfo.imageColorSpace = static_cast<VkColorSpaceKHR>(colorSpace);
            return *this;
        }

        SwapchainBuilder& SetPresentMode(SwapchainPresentMode presentMode) {
            m_createInfo.presentMode = static_cast<VkPresentModeKHR>(presentMode);
            return *this;
        }

        SwapchainBuilder& SetExtent(const VkExtent2D& extent) {
            m_createInfo.imageExtent = extent;
            return *this;
        }

        SwapchainBuilder& SetExtent(uint32_t width, uint32_t height) {
            m_createInfo.imageExtent = {width, height};
            return *this;
        }

        Swapchain build() {
            Swapchain swapchain;
            swapchain.m_device = m_device->GetRaw();
            swapchain.m_format = static_cast<ImageFormat>(m_createInfo.imageFormat);
            swapchain.m_colorSpace = static_cast<ColorSpace>(m_createInfo.imageColorSpace);
            swapchain.m_presentMode = static_cast<SwapchainPresentMode>(m_createInfo.presentMode);
            swapchain.m_extent = m_createInfo.imageExtent;
            VkSwapchainKHR swapchainHandle = VK_NULL_HANDLE;
            CheckVk(vkCreateSwapchainKHR(m_device->GetRaw(), &m_createInfo, nullptr, &swapchainHandle));
            swapchain.m_swapchain = swapchainHandle;

            CheckVk(vkGetSwapchainImagesKHR(m_device->GetRaw(), swapchain.m_swapchain, &m_createInfo.minImageCount, nullptr));
            swapchain.m_images.resize(m_createInfo.minImageCount);
            CheckVk(vkGetSwapchainImagesKHR(m_device->GetRaw(), swapchain.m_swapchain, &m_createInfo.minImageCount, swapchain.m_images.data()));

            swapchain.m_imageViews.resize(swapchain.m_images.size());
            for (size_t i = 0; i < swapchain.m_images.size(); ++i) {
                VkImageViewCreateInfo viewCreateInfo{
                    .sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO,
                    .image = swapchain.m_images[i],
                    .viewType = VK_IMAGE_VIEW_TYPE_2D,
                    .format = m_createInfo.imageFormat,
                    .subresourceRange = {
                        .aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
                        .levelCount = 1,
                        .layerCount = 1
                    }
                };
                CheckVk(vkCreateImageView(m_device->GetRaw(), &viewCreateInfo, nullptr, &swapchain.m_imageViews[i]));
            }
            return swapchain;
        }
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
        Device* m_device;
    };
}
