
#include <algorithm>
#include "nv_helpers.h"
#include "nv_device.h"
#include "nv_logger.h"

namespace {
    bool sCreateLogicalDevice(const Nova::PhysicalDevice& physicalDevice, uint32_t graphicsQueueFamilyIndex, VkDevice& device)
    {
        const float qfPriority = 1.0f;
        VkDeviceQueueCreateInfo queueCreateInfo{
            .sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO,
            .queueFamilyIndex = graphicsQueueFamilyIndex,
            .queueCount = 1,
            .pQueuePriorities = &qfPriority
        };
        VkPhysicalDeviceVulkan12Features vk12Features{
            .sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_2_FEATURES,
            .descriptorIndexing = true,
            .shaderSampledImageArrayNonUniformIndexing = true,
            .descriptorBindingVariableDescriptorCount = true,
            .runtimeDescriptorArray = true,
            .bufferDeviceAddress = true
        };
        const VkPhysicalDeviceVulkan13Features vk13Features{
            .sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_3_FEATURES,
            .pNext = &vk12Features,
            .synchronization2 = true,
            .dynamicRendering = true,
        };
        const VkPhysicalDeviceFeatures vk10Features{
            .samplerAnisotropy = VK_TRUE
        };

        std::vector<const char*> deviceExtensions = { VK_KHR_SWAPCHAIN_EXTENSION_NAME };
        if (PlatformDefined(Platform::MacOS)) {
            deviceExtensions.push_back(VK_KHR_PORTABILITY_SUBSET_EXTENSION_NAME);
        }

        VkDeviceCreateInfo deviceCreateInfo{
            .sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO,
            .pNext = &vk13Features,
            .queueCreateInfoCount = 1,
            .pQueueCreateInfos = &queueCreateInfo,
            .enabledExtensionCount = static_cast<uint32_t>(deviceExtensions.size()),
            .ppEnabledExtensionNames = deviceExtensions.data(),
            .pEnabledFeatures = &vk10Features
        };
        device = VK_NULL_HANDLE;
        return vkCreateDevice(physicalDevice.GetRaw(), &deviceCreateInfo, nullptr, &device) == VK_SUCCESS;
    }
}

namespace Nova {
    Device::Device(const Instance& instance, const PhysicalDevice& physicalDevice)
        : m_instance(&instance), m_physicalDevice(&physicalDevice)
    {
        if (m_physicalDevice == nullptr) {
            LogError("No GPU found.");
            exit(-1);
        }
        
        const PhysicalDevice& selectedDevice = *m_physicalDevice;
        LogInfo("Creating Nova Device with GPU: {} (type: {}, VRAM: {} MB)",
            selectedDevice.GetDeviceName(),
            static_cast<int>(selectedDevice.GetType()),
            selectedDevice.GetVRAMSize() / (1024 * 1024));

        bool success = sCreateLogicalDevice(selectedDevice, selectedDevice.GetGraphicsQueueFamilyIndex(), m_device);
        if (!success) {
            LogError("Failed to create logical device.");
        }
    }

    Device::Device(const Instance& instance)
        : Device(instance, instance.GetBestPhysicalDevice())
    {

    }

    Device::~Device()
    {
        if (m_device != VK_NULL_HANDLE) {
            CheckVk(vkDeviceWaitIdle(m_device));
            DestroyAllocator();
            vkDestroyDevice(m_device, nullptr);
            m_device = VK_NULL_HANDLE;
        }
    }

    std::vector<ImageFormat> Device::GetSupportedDepthFormats() const
    {
        return m_physicalDevice->GetSupportedDepthFormats();
    }

    SwapchainBuilder Device::CreateSwapchainBuilder(const Surface& surface)
    {
        return SwapchainBuilder(*this, surface);
    }

    Buffer Device::CreateBuffer(BufferSize size, BufferUsageFlag usageFlags, AllocationFlag allocationFlags) const
    {
        VkBufferCreateInfo bufferCreateInfo{
            .sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO,
            .size = size,
            .usage = static_cast<VkBufferUsageFlags>(usageFlags)
        };

        BufferAllocation allocation = CreateBufferAllocation(*m_instance, *this, bufferCreateInfo, static_cast<AllocationFlag>(allocationFlags));

        Buffer buffer{};
        buffer.m_allocation = std::move(allocation);
        buffer.m_size = size;
        buffer.m_usageFlags = usageFlags;
        buffer.m_device = m_device;

        if ((usageFlags & BufferUsageFlag::ShaderDeviceAddress) != BufferUsageFlag::None) {
            VkBufferDeviceAddressInfo bufferDeviceAddressInfo{
                .sType = VK_STRUCTURE_TYPE_BUFFER_DEVICE_ADDRESS_INFO,
                .buffer = buffer.GetRaw()
            };
            VkDeviceAddress address = vkGetBufferDeviceAddress(m_device, &bufferDeviceAddressInfo);
            buffer.m_deviceAddress = static_cast<BufferDeviceAddress>(address);
        }

        return buffer;
    }

    ImageBuilder Device::CreateImageBuilder() const
    {
        return ImageBuilder(*this);
    }

    Fence Device::CreateFence(bool signaled) const
    {
        VkFenceCreateInfo fenceCreateInfo{
            .sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO,
            .flags = signaled ? VK_FENCE_CREATE_SIGNALED_BIT : static_cast<VkFenceCreateFlags>(0)
        };

        Fence fence{};
        fence.device = m_device;
        CheckVk(vkCreateFence(m_device, &fenceCreateInfo, nullptr, &fence.fence));

        return fence;
    }

    Semaphore Device::CreateSemaphore() const
    {
        static const VkSemaphoreCreateInfo semaphoreCreateInfo{
            .sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO
        };

        Semaphore semaphore{};
        semaphore.device = m_device;
        CheckVk(vkCreateSemaphore(m_device, &semaphoreCreateInfo, nullptr, &semaphore.semaphore));

        return semaphore;
    }

    CommandPool Device::CreateCommandPool(CommandPoolCreateFlag flags) const
    {
        return CreateCommandPool(GetPhysicalDevice().GetGraphicsQueueFamilyIndex(), flags);
    }

    CommandPool Device::CreateCommandPool(uint32_t queueFamilyIndex, CommandPoolCreateFlag flags) const
    {
        VkCommandPoolCreateInfo commandPoolCreateInfo{
            .sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO,
            .queueFamilyIndex = queueFamilyIndex,
            .flags = static_cast<VkCommandPoolCreateFlags>(flags)
        };

        CommandPool commandPool{};
        commandPool.m_device = m_device;
        CheckVk(vkCreateCommandPool(m_device, &commandPoolCreateInfo, nullptr, &commandPool.m_commandPool));

        return commandPool;
    }

    CommandBuffer Device::CreateCommandBuffer(const CommandPool& commandPool) const
    {
        VkCommandBufferAllocateInfo commandBufferAllocateInfo{
            .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO,
            .commandPool = commandPool.GetRaw(),
            .level = VK_COMMAND_BUFFER_LEVEL_PRIMARY,
            .commandBufferCount = 1
        };

        CommandBuffer commandBuffer{};
        commandBuffer.m_device = m_device;
        commandBuffer.m_commandPool = &commandPool;
        CheckVk(vkAllocateCommandBuffers(m_device, &commandBufferAllocateInfo, &commandBuffer.m_commandBuffer));

        return commandBuffer;
    }

    std::vector<CommandBuffer> Device::CreateCommandBuffers(const CommandPool& commandPool, size_t count) const
    {
        VkCommandBufferAllocateInfo commandBufferAllocateInfo{
            .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO,
            .commandPool = commandPool.GetRaw(),
            .level = VK_COMMAND_BUFFER_LEVEL_PRIMARY,
            .commandBufferCount = static_cast<uint32_t>(count)
        };

        std::vector<VkCommandBuffer> commandBuffers(count);
        CheckVk(vkAllocateCommandBuffers(m_device, &commandBufferAllocateInfo, commandBuffers.data()));

        std::vector<CommandBuffer> novaCommandBuffers;
        novaCommandBuffers.reserve(count);
        for (size_t i = 0; i < count; ++i) {
            CommandBuffer novaCommandBuffer{};
            novaCommandBuffer.m_device = m_device;
            novaCommandBuffer.m_commandPool = &commandPool;
            novaCommandBuffer.m_commandBuffer = commandBuffers[i];
            novaCommandBuffers.push_back(std::move(novaCommandBuffer));
        }
        return novaCommandBuffers;
    }

    void Device::SubmitCommandBuffer(CommandBuffer* commandBuffer, Fence* fence) const
    {
        if (commandBuffer == nullptr || commandBuffer->GetRaw() == VK_NULL_HANDLE) {
            LogError("Invalid command buffer.");
            return;
        }
        VkCommandBuffer rawCommandBuffer = commandBuffer->GetRaw();
        VkSubmitInfo submitInfo{
            .sType = VK_STRUCTURE_TYPE_SUBMIT_INFO,
            .commandBufferCount = 1,
            .pCommandBuffers = &rawCommandBuffer
        };

        VkQueue graphicsQueue;
        vkGetDeviceQueue(m_device, GetPhysicalDevice().GetGraphicsQueueFamilyIndex(), 0, &graphicsQueue);
        CheckVk(vkQueueSubmit(graphicsQueue, 1, &submitInfo, fence ? fence->fence : VK_NULL_HANDLE));
    }

    void Device::WaitForFence(Fence* fence, uint64_t timeout) const
    {
        if (fence == nullptr || fence->fence == VK_NULL_HANDLE) {
            LogError("Invalid fence.");
            return;
        }
        CheckVk(vkWaitForFences(m_device, 1, &fence->fence, VK_TRUE, timeout));
    }
}
