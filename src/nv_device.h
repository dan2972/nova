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
#include "nv_texture.h"

namespace Nova {
    class Device {
    public:
        Device(const Instance& instance);
        Device(const Instance& instance, const PhysicalDevice& physicalDevice);
        ~Device();

        Device(const Device&) = delete;
        Device& operator=(const Device&) = delete;
        Device(Device&&) = delete;
        Device& operator=(Device&&) = delete;

        VkDevice GetRaw() const { return m_device; }

        const Instance& GetInstance() const { return *m_instance; }
        const PhysicalDevice& GetPhysicalDevice() const { return *m_physicalDevice; }

        std::vector<ImageFormat> GetSupportedDepthFormats() const;
        SwapchainBuilder CreateSwapchainBuilder(const Surface& surface);

        Buffer CreateBuffer(BufferSize size, BufferUsageFlag usageFlags, AllocationFlag allocationFlags = AllocationFlag::None) const;
        Image CreateImage(const ImageCreateInfo& createInfo) const;

        Fence CreateFence(bool signaled = false) const;
        Semaphore CreateSemaphore() const;

        CommandPool CreateCommandPool(CommandPoolCreateFlag flags = CommandPoolCreateFlag::None) const;
        CommandPool CreateCommandPool(uint32_t queueFamilyIndex, CommandPoolCreateFlag flags = CommandPoolCreateFlag::None) const;

        CommandBuffer CreateCommandBuffer(const CommandPool& commandPool) const;
        std::vector<CommandBuffer> CreateCommandBuffers(const CommandPool& commandPool, size_t count) const;

        void SubmitCommandBuffer(CommandBuffer* commandBuffer, Fence* fence = nullptr) const;
        void WaitForFence(Fence* fence, uint64_t timeout = UINT64_MAX) const;

        Texture CreateFromFile(const std::string& filePath, const CommandPool& commandPool);
    private:
        const Instance* m_instance;
        const PhysicalDevice* m_physicalDevice;
        VkDevice m_device = VK_NULL_HANDLE;
    };
}
