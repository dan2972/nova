#include "nv_image.h"

#include "nv_device.h"
#include "nv_helpers.h"

namespace Nova {
    Image::Image(const Device &device, const ImageCreateInfo &createInfo)
    {
        VkImageCreateInfo imageCreateInfo{
            .sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO,
            .imageType = VK_IMAGE_TYPE_2D,
            .format = static_cast<VkFormat>(createInfo.format),
            .extent = createInfo.extent,
            .mipLevels = createInfo.mipLevels,
            .arrayLayers = 1,
            .samples = VK_SAMPLE_COUNT_1_BIT,
            .tiling = VK_IMAGE_TILING_OPTIMAL,
            .usage = static_cast<VkImageUsageFlags>(createInfo.usage),
            // .sharingMode = VK_SHARING_MODE_EXCLUSIVE,
            .initialLayout = VK_IMAGE_LAYOUT_UNDEFINED
        };

        m_device = &device;
        m_extent = createInfo.extent;
        m_format = createInfo.format;
        m_allocation = CreateImageAllocation(device.GetInstance(), device, imageCreateInfo, createInfo.allocationFlags);
        m_usageFlags = createInfo.usage;
        m_mipLevels = createInfo.mipLevels;
    }

    ImageView Image::CreateView() const
    {
        VkImageAspectFlags aspectMask = 0;
        VkImageUsageFlags usageFlags = static_cast<VkImageUsageFlags>(m_usageFlags);
        if (usageFlags & VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT) {
            aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        } else if (usageFlags & VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT) {
            aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT;
            if (m_format == ImageFormat::D32SfloatS8Uint || m_format == ImageFormat::D24UnormS8Uint) {
                aspectMask |= VK_IMAGE_ASPECT_STENCIL_BIT;
            }
        } else if (usageFlags & VK_IMAGE_USAGE_SAMPLED_BIT) {
            aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        }

        return CreateView(static_cast<ImageAspectFlag>(aspectMask));
    }

    ImageView Image::CreateView(ImageAspectFlag aspectMask) const
    {
        if (m_allocation.image == VK_NULL_HANDLE) {
            LogError("Cannot create image view: image is not allocated.");
            return {};
        }
        
        VkImageView vkImageView = VK_NULL_HANDLE;
        VkImageViewCreateInfo viewCreateInfo{
            .sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO,
            .image = m_allocation.image,
            .viewType = VK_IMAGE_VIEW_TYPE_2D,
            .format = static_cast<VkFormat>(m_format),
            .subresourceRange = {
                .aspectMask = static_cast<VkImageAspectFlags>(aspectMask),
                .baseMipLevel = 0,
                .levelCount = m_mipLevels,
                .baseArrayLayer = 0,
                .layerCount = 1
            }
        };
        CheckVk(vkCreateImageView(m_device->GetRaw(), &viewCreateInfo, nullptr, &vkImageView));

        ImageView imageView{};
        imageView.m_imageView = vkImageView;
        imageView.m_device = m_device;
        return imageView;
    }
    
    void ImageView::Destroy()
    {
        if (m_imageView != VK_NULL_HANDLE) {
            vkDestroyImageView(m_device->GetRaw(), m_imageView, nullptr);
            m_imageView = VK_NULL_HANDLE;
        }
    }
}
