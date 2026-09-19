#include "nv_swapchain.h"
#include "nv_device.h"

namespace Nova {
    SwapchainBuilder::SwapchainBuilder(const Device& device, const Surface& surface) : m_device(&device) {
        VkSurfaceCapabilitiesKHR surfaceCaps = device.GetPhysicalDevice().GetSurfaceCapabilities(surface);
        m_createInfo.surface = surface.GetRaw();
        m_createInfo.minImageCount = surfaceCaps.minImageCount;
        m_createInfo.imageExtent = surfaceCaps.currentExtent;
    }

    Swapchain SwapchainBuilder::build() {
        Swapchain swapchain;
        swapchain.m_device = m_device;
        swapchain.m_format = static_cast<ImageFormat>(m_createInfo.imageFormat);
        swapchain.m_colorSpace = static_cast<ColorSpace>(m_createInfo.imageColorSpace);
        swapchain.m_presentMode = static_cast<SwapchainPresentMode>(m_createInfo.presentMode);
        swapchain.m_extent = {m_createInfo.imageExtent.width, m_createInfo.imageExtent.height};
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

    void Swapchain::Destroy()
    {
        if (m_device == nullptr) return;

        for (VkImageView imageView : m_imageViews) {
            if (imageView != VK_NULL_HANDLE) {
                vkDestroyImageView(m_device->GetRaw(), imageView, nullptr);
            }
        }
        m_imageViews.clear();
        m_images.clear();

        if (m_swapchain != VK_NULL_HANDLE) {
            vkDestroySwapchainKHR(m_device->GetRaw(), m_swapchain, nullptr);
            m_swapchain = VK_NULL_HANDLE;
        }
        m_device = nullptr;
    }
}