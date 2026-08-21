#pragma once

#include <volk.h>
#include <vk_mem_alloc.h>
#include "nv_instance.h"
#include "nv_device.h"

namespace Nova {

    struct BufferAllocation {
        VmaAllocation allocation;
        VkBuffer buffer;

        void Destroy(VkDevice device);
    };

    struct ImageAllocation {
        VmaAllocation allocation;
        VkImage image;

        void Destroy(VkDevice device);
    };

    ImageAllocation CreateImage(const Nova::Instance& instance, const Nova::Device& device, VkImageCreateInfo imageCreateInfo, VkImage* image);
    BufferAllocation CreateBuffer(const Nova::Instance& instance, const Nova::Device& device, VkBufferCreateInfo bufferCreateInfo, VkBuffer* buffer);
}