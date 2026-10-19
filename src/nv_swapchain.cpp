#include "nv_swapchain.h"
#include "nv_device.h"

namespace nova {
    SwapchainBuilder::SwapchainBuilder(const Device& device, const Surface& surface) : m_device(&device), m_surface(&surface) {
        VkSurfaceCapabilitiesKHR surfaceCaps = device.GetPhysicalDevice().GetSurfaceCapabilities(surface);
        m_createInfo.surface = surface.GetRaw();
        m_createInfo.minImageCount = surfaceCaps.minImageCount;
        m_createInfo.imageExtent = surfaceCaps.currentExtent;
    }

    SwapchainBuilder &SwapchainBuilder::SetDesiredFormat(ImageFormat format)
    {
        // Check if the format is supported
        uint32_t formatCount = 0;
        CheckVk(vkGetPhysicalDeviceSurfaceFormatsKHR(m_device->GetPhysicalDevice().GetRaw(), m_createInfo.surface, &formatCount, nullptr));
        std::vector<VkSurfaceFormatKHR> surfaceFormats(formatCount);
        CheckVk(vkGetPhysicalDeviceSurfaceFormatsKHR(m_device->GetPhysicalDevice().GetRaw(), m_createInfo.surface, &formatCount, surfaceFormats.data()));
        for (const auto& surfaceFormat : surfaceFormats) {
            if (surfaceFormat.format == static_cast<VkFormat>(format)) {
                m_createInfo.imageFormat = static_cast<VkFormat>(format);
                break;
            }
        }
        // If the format is not supported, fallback to B8G8R8A8 SRGB
        if (m_createInfo.imageFormat != static_cast<VkFormat>(format)) {
            m_createInfo.imageFormat = VK_FORMAT_B8G8R8A8_SRGB;
        }
        return *this;
    }

    SwapchainBuilder &SwapchainBuilder::SetDesiredColorSpace(ColorSpace colorSpace)
    {
        // Check if the color space is supported
        VkSurfaceCapabilitiesKHR surfaceCaps = m_device->GetPhysicalDevice().GetSurfaceCapabilities(*m_surface);
        if (surfaceCaps.supportedCompositeAlpha & static_cast<VkCompositeAlphaFlagBitsKHR>(colorSpace)) {
            m_createInfo.imageColorSpace = static_cast<VkColorSpaceKHR>(colorSpace);
        } else {
            // Fallback to SRGB Nonlinear if the requested color space is not supported
            m_createInfo.imageColorSpace = VK_COLOR_SPACE_SRGB_NONLINEAR_KHR;
        }
        return *this;
    }

    SwapchainBuilder &SwapchainBuilder::SetDesiredPresentMode(SwapchainPresentMode presentMode)
    {
        // Check if the present mode is supported
        uint32_t presentModeCount = 0;
        CheckVk(vkGetPhysicalDeviceSurfacePresentModesKHR(m_device->GetPhysicalDevice().GetRaw(), m_createInfo.surface, &presentModeCount, nullptr));
        std::vector<VkPresentModeKHR> presentModes(presentModeCount);
        CheckVk(vkGetPhysicalDeviceSurfacePresentModesKHR(m_device->GetPhysicalDevice().GetRaw(), m_createInfo.surface, &presentModeCount, presentModes.data()));
        for (const auto& mode : presentModes) {
            if (mode == static_cast<VkPresentModeKHR>(presentMode)) {
                m_createInfo.presentMode = mode;
                break;
            }
        }
        // Fallback to FIFO if the requested present mode is not supported
        if (m_createInfo.presentMode != static_cast<VkPresentModeKHR>(presentMode)) {
            m_createInfo.presentMode = VK_PRESENT_MODE_FIFO_KHR;
        }
        return *this;
    }

    SwapchainBuilder &SwapchainBuilder::SetDesiredExtent(const Extent2D &extent)
    {
        // Check if the extent is within the supported range
        VkSurfaceCapabilitiesKHR surfaceCaps = m_device->GetPhysicalDevice().GetSurfaceCapabilities(*m_surface);
        if (extent.width >= surfaceCaps.minImageExtent.width && extent.width <= surfaceCaps.maxImageExtent.width &&
            extent.height >= surfaceCaps.minImageExtent.height && extent.height <= surfaceCaps.maxImageExtent.height) {
            m_createInfo.imageExtent = extent;
        } else {
            // Fallback to the current extent if the requested extent is not supported
            m_createInfo.imageExtent = surfaceCaps.currentExtent;
        }
        return *this;
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

    Image Swapchain::AcquireNextImage(uint32_t& imageIndex, const Semaphore *semaphore, const Fence *fence, uint64_t timeout) const
    {
        VkResult result = vkAcquireNextImageKHR(m_device->GetRaw(), m_swapchain, timeout, semaphore ? semaphore->GetRaw() : VK_NULL_HANDLE, fence ? fence->GetRaw() : VK_NULL_HANDLE, &imageIndex);
        if (result != VK_SUCCESS) {
            LogError("Failed to acquire next image from swapchain.");
            return Image();
        }
        Image image;
        image.m_device = m_device;
        image.m_allocation.allocation = VK_NULL_HANDLE;
        image.m_allocation.image = m_images[imageIndex];
        image.m_format = m_format;
        image.m_extent = {m_extent.width, m_extent.height, 1};
        return image;
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

    std::vector<Image> Swapchain::GetImages() const
    {
        std::vector<Image> images;
        images.reserve(m_images.size());
        for (const VkImage& vkImage : m_images) {
            Image image;
            image.m_device = m_device;
            image.m_allocation.allocation = VK_NULL_HANDLE;
            image.m_allocation.image = vkImage;
            image.m_format = m_format;
            image.m_extent = {m_extent.width, m_extent.height, 1};
            image.m_usageFlags = ImageUsageFlag::ColorAttachment;
            image.m_mipLevels = 1;
            images.push_back(std::move(image));
        }
        return images;
    }

    std::vector<ImageView> Swapchain::GetImageViews() const
    {
        std::vector<ImageView> imageViews;
        imageViews.reserve(m_imageViews.size());
        for (const VkImageView& vkImageView : m_imageViews) {
            ImageView imageView;
            imageView.m_device = m_device;
            imageView.m_imageView = vkImageView;
            imageView.m_ownsImageView = false;
            imageViews.push_back(std::move(imageView));
        }
        return imageViews;
    }
}
