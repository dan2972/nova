#include "nv_image.h"

#include "nv_device.h"
#include "nv_helpers.h"

namespace Nova {
    Image ImageBuilder::build(const Instance& instance, AllocationFlag allocationFlags) {
        Image image{};
        image.m_extent = m_createInfo.extent;
        image.m_format = static_cast<ImageFormat>(m_createInfo.format);
        image.m_allocation = CreateImageAllocation(instance, *m_device, m_createInfo, allocationFlags);
        image.m_device = m_device->GetRaw();
        image.m_usageFlags = m_createInfo.usage;
        image.m_mipLevels = m_createInfo.mipLevels;
        
        return image;
    }

    ImageView Image::CreateView() const
    {
        VkImageAspectFlags aspectMask = 0;
        if (m_usageFlags & VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT) {
            aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        } else if (m_usageFlags & VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT) {
            aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT;
            if (m_format == ImageFormat::D32SfloatS8Uint || m_format == ImageFormat::D24UnormS8Uint) {
                aspectMask |= VK_IMAGE_ASPECT_STENCIL_BIT;
            }
        } else if (m_usageFlags & VK_IMAGE_USAGE_SAMPLED_BIT) {
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
        CheckVk(vkCreateImageView(m_device, &viewCreateInfo, nullptr, &vkImageView));

        ImageView imageView{};
        imageView.m_imageView = vkImageView;
        imageView.m_device = m_device;
        return imageView;
    }
}
