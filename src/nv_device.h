#pragma once

#include <volk.h>
#include <vector>
#include <glm/glm.hpp>
#include "nv_instance.h"
#include "nv_physicalDevice.h"
#include "nv_window.h"
#include "nv_swapchain.h"
#include "nv_image.h"
#include "nv_buffer.h"
#include "nv_allocator.h"
#include "nv_synchronization.h"
#include "nv_commandBuffer.h"

namespace Nova {
    class Device {
    public:
        Device(Instance* instance, PhysicalDevice* physicalDevice = nullptr);
        ~Device();

        Device(const Device&) = delete;
        Device& operator=(const Device&) = delete;
        Device(Device&&) = delete;
        Device& operator=(Device&&) = delete;

        VkDevice GetRaw() const { return m_device; }

        const Instance* GetInstance() const { return m_instance; }
        const PhysicalDevice* GetPhysicalDevice() const { return m_physicalDevice; }

        std::vector<ImageFormat> GetSupportedDepthFormats() const;

        SwapchainBuilder CreateSwapchainBuilder(const Surface* surface);

        Buffer CreateBuffer(BufferSize size, BufferUsageFlag usageFlags, AllocationFlag allocationFlags = AllocationFlag::HostAccessSequentialWrite | AllocationFlag::HostAccessAllowTransfer | AllocationFlag::Mapped);

        ImageBuilder CreateImageBuilder();

        Fence CreateFence(bool signaled = false);
        Semaphore CreateSemaphore();

        CommandPool CreateCommandPool(CommandPoolCreateFlag flags = CommandPoolCreateFlag::None);
        CommandPool CreateCommandPool(uint32_t queueFamilyIndex, CommandPoolCreateFlag flags = CommandPoolCreateFlag::None);

        CommandBuffer CreateCommandBuffer(CommandPool& commandPool);
        std::vector<CommandBuffer> CreateCommandBuffers(CommandPool& commandPool, size_t count);

        void SubmitCommandBuffer(CommandBuffer* commandBuffer, Fence* fence = nullptr);
        void WaitForFence(Fence* fence, uint64_t timeout = UINT64_MAX);
    private:
        const Instance* m_instance;
        const PhysicalDevice* m_physicalDevice;
        VkDevice m_device = VK_NULL_HANDLE;
    };
}
