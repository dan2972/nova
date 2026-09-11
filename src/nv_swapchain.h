#pragma once

#include <volk.h>
#include <vector>

#include "nv_device.h"
#include "nv_helpers.h"

namespace Nova {
    enum class ImageFormat {
        R8G8B8A8Unorm = VK_FORMAT_R8G8B8A8_UNORM,
        B8G8R8A8Unorm = VK_FORMAT_B8G8R8A8_UNORM,
        R16G16B16A16Sfloat = VK_FORMAT_R16G16B16A16_SFLOAT,
        A2B10G10R10UnormPack32 = VK_FORMAT_A2B10G10R10_UNORM_PACK32
    };

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

    struct Swapchain {
        VkSwapchainKHR swapchain;
        ImageFormat format;
        ColorSpace colorSpace;
        SwapchainPresentMode presentMode;
        VkExtent2D extent;
        std::vector<VkImage> images;
        std::vector<VkImageView> imageViews;
    };

    struct SwapchainBuilder {
        VkSwapchainCreateInfoKHR createInfo{
            .sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR,
            .imageFormat = VK_FORMAT_B8G8R8A8_SRGB,
            .imageColorSpace = VK_COLOR_SPACE_SRGB_NONLINEAR_KHR,
            .imageArrayLayers = 1,
            .imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT,
            .preTransform = VK_SURFACE_TRANSFORM_IDENTITY_BIT_KHR,
            .compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR,
            .presentMode = VK_PRESENT_MODE_FIFO_KHR,
        };
        Device* device;

        SwapchainBuilder(Device* device) : device(device) {
            VkSurfaceCapabilitiesKHR surfaceCaps = device->GetSurfaceCapabilities();
            createInfo.surface = device->GetSurface();
            createInfo.minImageCount = surfaceCaps.minImageCount;
            createInfo.imageExtent = surfaceCaps.currentExtent;
        }

        SwapchainBuilder& SetFormat(ImageFormat format) {
            createInfo.imageFormat = static_cast<VkFormat>(format);
            return *this;
        }

        SwapchainBuilder& SetColorSpace(ColorSpace colorSpace) {
            createInfo.imageColorSpace = static_cast<VkColorSpaceKHR>(colorSpace);
            return *this;
        }

        SwapchainBuilder& SetPresentMode(SwapchainPresentMode presentMode) {
            createInfo.presentMode = static_cast<VkPresentModeKHR>(presentMode);
            return *this;
        }

        SwapchainBuilder& SetExtent(const VkExtent2D& extent) {
            createInfo.imageExtent = extent;
            return *this;
        }

        SwapchainBuilder& SetExtent(uint32_t width, uint32_t height) {
            createInfo.imageExtent = {width, height};
            return *this;
        }

        const Swapchain build() {
            Swapchain swapchain;
            swapchain.format = static_cast<ImageFormat>(createInfo.imageFormat);
            swapchain.colorSpace = static_cast<ColorSpace>(createInfo.imageColorSpace);
            swapchain.presentMode = static_cast<SwapchainPresentMode>(createInfo.presentMode);
            swapchain.extent = createInfo.imageExtent;
            VkSwapchainKHR swapchainHandle = VK_NULL_HANDLE;
            CheckVk(vkCreateSwapchainKHR(device->GetRaw(), &createInfo, nullptr, &swapchainHandle));
            swapchain.swapchain = swapchainHandle;

            CheckVk(vkGetSwapchainImagesKHR(device->GetRaw(), swapchain.swapchain, &createInfo.minImageCount, nullptr));
            swapchain.images.resize(createInfo.minImageCount);
            CheckVk(vkGetSwapchainImagesKHR(device->GetRaw(), swapchain.swapchain, &createInfo.minImageCount, swapchain.images.data()));

            swapchain.imageViews.resize(swapchain.images.size());
            for (size_t i = 0; i < swapchain.images.size(); ++i) {
                VkImageViewCreateInfo viewCreateInfo{
                    .sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO,
                    .image = swapchain.images[i],
                    .viewType = VK_IMAGE_VIEW_TYPE_2D,
                    .format = createInfo.imageFormat,
                    .subresourceRange = {
                        .aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
                        .levelCount = 1,
                        .layerCount = 1
                    }
                };
                CheckVk(vkCreateImageView(device->GetRaw(), &viewCreateInfo, nullptr, &swapchain.imageViews[i]));
            }
            return swapchain;
        }
    };
}