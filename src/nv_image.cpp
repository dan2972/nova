#include "nv_image.h"

#include "nv_device.h"
#include "nv_helpers.h"

namespace Nova {
    Image ImageBuilder::build(const Instance& instance) {
        Image image{};
        image.extent = createInfo.extent;
        image.format = static_cast<ImageFormat>(createInfo.format);
        image.allocation = CreateImageAllocation(instance, *device, createInfo);
        image.device = device->GetRaw();

        VkImageAspectFlags aspectMask = 0;
        if (createInfo.usage & VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT) {
            aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        } else if (createInfo.usage & VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT) {
            aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT;
            if (createInfo.format == VK_FORMAT_D32_SFLOAT_S8_UINT || createInfo.format == VK_FORMAT_D24_UNORM_S8_UINT) {
                aspectMask |= VK_IMAGE_ASPECT_STENCIL_BIT;
            }
        } else if (createInfo.usage & VK_IMAGE_USAGE_SAMPLED_BIT) {
            aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        }

        VkImageViewCreateInfo viewCreateInfo{
            .sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO,
            .image = image.allocation.image,
            .viewType = VK_IMAGE_VIEW_TYPE_2D,
            .format = createInfo.format,
            .subresourceRange = {
                .aspectMask = aspectMask,
                .baseMipLevel = 0,
                .levelCount = 1,
                .baseArrayLayer = 0,
                .layerCount = 1
            }
        };
        CheckVk(vkCreateImageView(device->GetRaw(), &viewCreateInfo, nullptr, &image.imageView));
        return image;
    }
}
