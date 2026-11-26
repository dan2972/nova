#include "nv_image.h"

#include "nv_device.h"
#include "nv_helpers.h"

namespace nova {
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

    ImageAspectFlag Image::GetAspectMask() const
    {
        switch (m_format) {
            case ImageFormat::D16Unorm:
            case ImageFormat::D32Sfloat: return ImageAspectFlag::Depth;
            case ImageFormat::D24UnormS8Uint:
            case ImageFormat::D32SfloatS8Uint: return ImageAspectFlag::Depth | ImageAspectFlag::Stencil;
            default: return ImageAspectFlag::Color;
        }
    }

    ImageView Image::CreateView() const
    {
        return CreateView(GetAspectMask());
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
        if (m_imageView != VK_NULL_HANDLE && m_ownsImageView) {
            vkDestroyImageView(m_device->GetRaw(), m_imageView, nullptr);
        }
        m_imageView = VK_NULL_HANDLE;
        m_device = nullptr;
        m_ownsImageView = false;
    }
}
