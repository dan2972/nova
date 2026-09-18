#pragma once

#include <utility>

#include <volk.h>
#include <vk_mem_alloc.h>

namespace Nova {
    class Instance;
    class Device;

    struct BufferAllocation {
        VmaAllocation allocation = VK_NULL_HANDLE;
        VmaAllocationInfo allocationInfo = {};
        VkBuffer buffer = VK_NULL_HANDLE;

        BufferAllocation() = default;
        BufferAllocation(const BufferAllocation&) = delete;
        BufferAllocation& operator=(const BufferAllocation&) = delete;

        BufferAllocation(BufferAllocation&& other) noexcept { *this = std::move(other); }
        BufferAllocation& operator=(BufferAllocation&& other) noexcept {
            if (this != &other) {
                allocation = other.allocation;
                allocationInfo = other.allocationInfo;
                buffer = other.buffer;
                other.allocation = VK_NULL_HANDLE;
                other.allocationInfo = {};
                other.buffer = VK_NULL_HANDLE;
            }
            return *this;
        }

        void Destroy(VkDevice device);
    };

    struct ImageAllocation {
        VmaAllocation allocation = VK_NULL_HANDLE;
        VkImage image = VK_NULL_HANDLE;

        ImageAllocation() = default;
        ImageAllocation(const ImageAllocation&) = delete;
        ImageAllocation& operator=(const ImageAllocation&) = delete;

        ImageAllocation(ImageAllocation&& other) noexcept { *this = std::move(other); }
        ImageAllocation& operator=(ImageAllocation&& other) noexcept {
            if (this != &other) {
                allocation = other.allocation;
                image = other.image;
                other.allocation = VK_NULL_HANDLE;
                other.image = VK_NULL_HANDLE;
            }
            return *this;
        }

        void Destroy(VkDevice device);
    };

    enum AllocationFlag : uint32_t {
        None = 0,
        DedicatedMemory = VMA_ALLOCATION_CREATE_DEDICATED_MEMORY_BIT,
        NeverAllocate = VMA_ALLOCATION_CREATE_NEVER_ALLOCATE_BIT,
        Mapped = VMA_ALLOCATION_CREATE_MAPPED_BIT,
        UserDataCopyString = VMA_ALLOCATION_CREATE_USER_DATA_COPY_STRING_BIT,
        UpperAddress = VMA_ALLOCATION_CREATE_UPPER_ADDRESS_BIT,
        DontBind = VMA_ALLOCATION_CREATE_DONT_BIND_BIT,
        WithinBudget = VMA_ALLOCATION_CREATE_WITHIN_BUDGET_BIT,
        CanAlias = VMA_ALLOCATION_CREATE_CAN_ALIAS_BIT,
        HostAccessSequentialWrite = VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT,
        HostAccessRandom= VMA_ALLOCATION_CREATE_HOST_ACCESS_RANDOM_BIT,
        HostAccessAllowTransfer = VMA_ALLOCATION_CREATE_HOST_ACCESS_ALLOW_TRANSFER_INSTEAD_BIT,
        StrategyMinMemory = VMA_ALLOCATION_CREATE_STRATEGY_MIN_MEMORY_BIT,
        StrategyMinTime = VMA_ALLOCATION_CREATE_STRATEGY_MIN_TIME_BIT,
        StrategyMinOffset = VMA_ALLOCATION_CREATE_STRATEGY_MIN_OFFSET_BIT,
        StrategyBestFit = VMA_ALLOCATION_CREATE_STRATEGY_BEST_FIT_BIT,
        StrategyFirstFit = VMA_ALLOCATION_CREATE_STRATEGY_FIRST_FIT_BIT,
        StrategyMask = VMA_ALLOCATION_CREATE_STRATEGY_MASK,
    };

    constexpr AllocationFlag operator|(AllocationFlag a, AllocationFlag b) {
        return static_cast<AllocationFlag>(static_cast<uint32_t>(a) | static_cast<uint32_t>(b));
    }

    constexpr AllocationFlag operator&(AllocationFlag a, AllocationFlag b) {
        return static_cast<AllocationFlag>(static_cast<uint32_t>(a) & static_cast<uint32_t>(b));
    }

    constexpr AllocationFlag operator~(AllocationFlag a) {
        return static_cast<AllocationFlag>(~static_cast<uint32_t>(a));
    }

    ImageAllocation CreateImageAllocation(const Instance& instance, const Device& device, const VkImageCreateInfo& imageCreateInfo, AllocationFlag flags = AllocationFlag::DedicatedMemory);
    BufferAllocation CreateBufferAllocation(const Instance& instance, const Device& device, const VkBufferCreateInfo& bufferCreateInfo, AllocationFlag flags = AllocationFlag::DedicatedMemory);

    void DestroyAllocator();
}
