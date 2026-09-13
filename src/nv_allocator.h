#pragma once

#include <volk.h>
#include <vk_mem_alloc.h>

namespace Nova {
    class Instance;
    class Device;

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

    ImageAllocation CreateImageAllocation(const Instance& instance, const Device& device, VkImageCreateInfo imageCreateInfo);
    BufferAllocation CreateBufferAllocation(const Instance& instance, const Device& device, VkBufferCreateInfo bufferCreateInfo);
}
