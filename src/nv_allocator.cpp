#include "nv_allocator.h"
#include "nv_device.h"
#include "nv_instance.h"
#include "nv_helpers.h"

namespace {
    static VmaAllocator s_allocator = VK_NULL_HANDLE;
    static bool sVmaInitialized = false;

    void sCheckVmaInitialized(const Nova::Instance& instance, const Nova::Device& device) {
        if (!sVmaInitialized) {
            VmaVulkanFunctions vkFunctions = {};
            vkFunctions.vkGetInstanceProcAddr = vkGetInstanceProcAddr;
            vkFunctions.vkGetDeviceProcAddr = vkGetDeviceProcAddr;
            VmaAllocatorCreateInfo allocatorCI{
                .flags = VMA_ALLOCATOR_CREATE_BUFFER_DEVICE_ADDRESS_BIT, 
                .physicalDevice = device.GetPhysicalDevice().GetRaw(),
                .device = device.GetRaw(),
                .pVulkanFunctions = &vkFunctions,
                .instance = instance.GetRaw()
            };
            CheckVk(vmaCreateAllocator(&allocatorCI, &s_allocator));
            sVmaInitialized = true;
        }
    }
}

namespace Nova {

    void BufferAllocation::Destroy(VkDevice device) {
        if (allocation != VK_NULL_HANDLE) {
            vmaDestroyBuffer(s_allocator, buffer, allocation);
            allocation = VK_NULL_HANDLE;
            buffer = VK_NULL_HANDLE;
        }
    }

    void ImageAllocation::Destroy(VkDevice device) {
        if (allocation != VK_NULL_HANDLE) {
            vmaDestroyImage(s_allocator, image, allocation);
            allocation = VK_NULL_HANDLE;
            image = VK_NULL_HANDLE;
        }
    }

    ImageAllocation CreateImageAllocation(const Instance& instance, const Device& device, VkImageCreateInfo imageCreateInfo, AllocationFlag flags) {
        sCheckVmaInitialized(instance, device);
        VmaAllocationCreateInfo allocCI{
            .flags = static_cast<uint32_t>(flags),
            .usage = VMA_MEMORY_USAGE_AUTO
        };
        ImageAllocation imageAlloc{
            .allocation = nullptr,
            .image = VK_NULL_HANDLE
        };
        CheckVk(vmaCreateImage(s_allocator, &imageCreateInfo, &allocCI, &imageAlloc.image, &imageAlloc.allocation, nullptr));
        return imageAlloc;
    }

    BufferAllocation CreateBufferAllocation(const Instance& instance, const Device& device, VkBufferCreateInfo bufferCreateInfo, AllocationFlag flags) {
        sCheckVmaInitialized(instance, device);
        VmaAllocationCreateInfo allocCI{
            .flags = static_cast<uint32_t>(flags),
            .usage = VMA_MEMORY_USAGE_AUTO
        };
        BufferAllocation bufferAlloc{
            .allocation = nullptr,
            .allocationInfo = {},
            .buffer = VK_NULL_HANDLE
        };
        CheckVk(vmaCreateBuffer(s_allocator, &bufferCreateInfo, &allocCI, &bufferAlloc.buffer, &bufferAlloc.allocation, &bufferAlloc.allocationInfo));
        return bufferAlloc;
    }

    void DestroyAllocator()
    {
        if (s_allocator != VK_NULL_HANDLE) {
            vmaDestroyAllocator(s_allocator);
            s_allocator = VK_NULL_HANDLE;
            sVmaInitialized = false;
        }
    }
}
