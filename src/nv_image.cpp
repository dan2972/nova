#include "nv_image.h"

#include "nv_device.h"
#include "nv_helpers.h"

namespace Nova {
    Image ImageBuilder::build(const Instance& instance) {
        Image image{};
        image.m_extent = m_createInfo.extent;
        image.m_format = static_cast<ImageFormat>(m_createInfo.format);
        image.m_allocation = CreateImageAllocation(instance, *m_device, m_createInfo);
        image.m_device = m_device->GetRaw();

        VkImageAspectFlags aspectMask = 0;
        if (m_createInfo.usage & VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT) {
            aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        } else if (m_createInfo.usage & VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT) {
            aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT;
            if (m_createInfo.format == VK_FORMAT_D32_SFLOAT_S8_UINT || m_createInfo.format == VK_FORMAT_D24_UNORM_S8_UINT) {
                aspectMask |= VK_IMAGE_ASPECT_STENCIL_BIT;
            }
        } else if (m_createInfo.usage & VK_IMAGE_USAGE_SAMPLED_BIT) {
            aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        }

        VkImageViewCreateInfo viewCreateInfo{
            .sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO,
            .image = image.m_allocation.image,
            .viewType = VK_IMAGE_VIEW_TYPE_2D,
            .format = m_createInfo.format,
            .subresourceRange = {
                .aspectMask = aspectMask,
                .baseMipLevel = 0,
                .levelCount = 1,
                .baseArrayLayer = 0,
                .layerCount = 1
            }
        };
        CheckVk(vkCreateImageView(m_device->GetRaw(), &viewCreateInfo, nullptr, &image.m_imageView));
        return image;
    }
}
