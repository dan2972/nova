#pragma once

#include <volk.h>
#include <vector>
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
#include "nv_descriptors.h"
#include "nv_pipeline.h"

namespace nova {
    class Queue {
    public:
        VkQueue GetRaw() const { return m_queue; }
        uint32_t GetFamilyIndex() const { return m_familyIndex; }
    private:
        VkQueue m_queue = VK_NULL_HANDLE;
        uint32_t m_familyIndex = 0;

        friend class Device;
    };

    class Device {
    public:
        Device() = default;
        Device(const Instance& instance);
        Device(const Instance& instance, const PhysicalDevice& physicalDevice);
        ~Device();

        Device(const Device&) = delete;
        Device& operator=(const Device&) = delete;
        Device(Device&& other)
            : m_instance(other.m_instance)
            , m_physicalDevice(other.m_physicalDevice)
            , m_device(other.m_device)
        {
            other.m_instance = nullptr;
            other.m_physicalDevice = nullptr;
            other.m_device = VK_NULL_HANDLE;
        }
        Device& operator=(Device&& other)
        {
            if (this != &other) {
                m_instance = other.m_instance;
                m_physicalDevice = other.m_physicalDevice;
                m_device = other.m_device;
                other.m_instance = nullptr;
                other.m_physicalDevice = nullptr;
                other.m_device = VK_NULL_HANDLE;
            }
            return *this;
        }

        VkDevice GetRaw() const { return m_device; }

        const Instance& GetInstance() const { return *m_instance; }
        const PhysicalDevice& GetPhysicalDevice() const { return *m_physicalDevice; }

        Queue GetGraphicsQueue() const;

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

        void SubmitCommandBuffer(const CommandBuffer& commandBuffer, const Fence* fence = nullptr) const;
        void SubmitCommandBuffers(CommandBufferSubmitInfo submitInfo) const;
        void WaitForFence(const Fence& fence, uint64_t timeout = UINT64_MAX) const;
        void ResetFence(const Fence& fence) const;

        void WaitIdle() const;

        void Present(const Swapchain& swapchain, uint32_t imageIndex, bool& shouldUpdateSwapchain, const std::vector<const Semaphore*>& waitSemaphores = {}) const;

        Texture CreateKTXTextureFromFile(const std::string& filePath, const CommandPool& commandPool);

        BindlessTextureDescriptors CreateBindlessTextureDescriptors(const std::vector<TextureDescriptor>& textureDescriptors) const;

        GraphicsPipelineBuilder CreateGraphicsPipelineBuilder() const;
    private:
        const Instance* m_instance;
        const PhysicalDevice* m_physicalDevice;
        VkDevice m_device = VK_NULL_HANDLE;
    };
}
