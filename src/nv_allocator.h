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

    enum AllocationFlag : uint32_t {
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

    ImageAllocation CreateImageAllocation(const Instance& instance, const Device& device, VkImageCreateInfo imageCreateInfo, AllocationFlag flags = AllocationFlag::DedicatedMemory);
    BufferAllocation CreateBufferAllocation(const Instance& instance, const Device& device, VkBufferCreateInfo bufferCreateInfo, AllocationFlag flags = AllocationFlag::DedicatedMemory);
}
