
#include <algorithm>
#include "nv_helpers.h"
#include "nv_device.h"
#include "nv_logger.h"

namespace {
    bool sSupportsRequiredFeatures(const nova::PhysicalDevice& physicalDevice)
    {
        VkPhysicalDeviceVulkan12Features vk12Features{
            .sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_2_FEATURES,
        };
        VkPhysicalDeviceVulkan13Features vk13Features{
            .sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_3_FEATURES,
            .pNext = &vk12Features,
        };
        VkPhysicalDeviceFeatures2 features{
            .sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_FEATURES_2,
            .pNext = &vk13Features,
        };
        vkGetPhysicalDeviceFeatures2(physicalDevice.GetRaw(), &features);

        return features.features.samplerAnisotropy &&
               vk12Features.descriptorIndexing &&
               vk12Features.shaderSampledImageArrayNonUniformIndexing &&
               vk12Features.descriptorBindingVariableDescriptorCount &&
               vk12Features.runtimeDescriptorArray &&
               vk12Features.bufferDeviceAddress &&
               vk13Features.synchronization2 &&
               vk13Features.dynamicRendering;
    }

    bool sCreateLogicalDevice(const nova::PhysicalDevice& physicalDevice, uint32_t graphicsQueueFamilyIndex, VkDevice& device)
    {
        if (!sSupportsRequiredFeatures(physicalDevice)) {
            nova::LogError("GPU '{}' does not support nova's required bindless Vulkan features.",
                           physicalDevice.GetDeviceName());
            return false;
        }

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

namespace nova {
    Device::Device(const Instance& instance, const PhysicalDevice& physicalDevice)
        : m_instance(&instance), m_physicalDevice(&physicalDevice)
    {
        if (m_physicalDevice == nullptr) {
            LogError("No GPU found.");
            exit(-1);
        }
        
        const PhysicalDevice& selectedDevice = *m_physicalDevice;
        LogInfo("Creating nova Device with GPU: {} (type: {}, VRAM: {} MB)",
            selectedDevice.GetDeviceName(),
            static_cast<int>(selectedDevice.GetType()),
            selectedDevice.GetVRAMSize() / (1024 * 1024));

        bool success = sCreateLogicalDevice(selectedDevice, selectedDevice.GetGraphicsQueueFamilyIndex(), m_device);
        if (!success) {
            LogError("Failed to create logical device.");
            exit(-1);
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

    Queue Device::GetGraphicsQueue() const
    {
        Queue queue;
        queue.m_familyIndex = m_physicalDevice->GetGraphicsQueueFamilyIndex();
        vkGetDeviceQueue(m_device, queue.m_familyIndex, 0, &queue.m_queue);
        return queue;
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
        buffer.m_device = this;

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

    Image Device::CreateImage(const ImageCreateInfo &createInfo) const
    {
        return Image(*this, createInfo);
    }

    Fence Device::CreateFence(bool signaled) const
    {
        VkFenceCreateInfo fenceCreateInfo{
            .sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO,
            .flags = signaled ? VK_FENCE_CREATE_SIGNALED_BIT : static_cast<VkFenceCreateFlags>(0)
        };

        Fence fence{};
        fence.m_device = this;
        CheckVk(vkCreateFence(m_device, &fenceCreateInfo, nullptr, &fence.m_fence));

        return fence;
    }

    Semaphore Device::CreateSemaphore() const
    {
        static const VkSemaphoreCreateInfo semaphoreCreateInfo{
            .sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO
        };

        Semaphore semaphore{};
        semaphore.m_device = this;
        CheckVk(vkCreateSemaphore(m_device, &semaphoreCreateInfo, nullptr, &semaphore.m_semaphore));

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
            .flags = static_cast<VkCommandPoolCreateFlags>(flags),
            .queueFamilyIndex = queueFamilyIndex
        };

        CommandPool commandPool{};
        commandPool.m_device = this;
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
        commandBuffer.m_device = this;
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
            novaCommandBuffer.m_device = this;
            novaCommandBuffer.m_commandPool = &commandPool;
            novaCommandBuffer.m_commandBuffer = commandBuffers[i];
            novaCommandBuffers.push_back(std::move(novaCommandBuffer));
        }
        return novaCommandBuffers;
    }

    void Device::SubmitCommandBuffer(const CommandBuffer& commandBuffer, const Fence* fence) const
    {
        if (commandBuffer.GetRaw() == VK_NULL_HANDLE) {
            LogError("Invalid command buffer.");
            return;
        }
        VkCommandBuffer rawCommandBuffer = commandBuffer.GetRaw();
        VkSubmitInfo submitInfo{
            .sType = VK_STRUCTURE_TYPE_SUBMIT_INFO,
            .commandBufferCount = 1,
            .pCommandBuffers = &rawCommandBuffer
        };

        CheckVk(vkQueueSubmit(GetGraphicsQueue().GetRaw(), 1, &submitInfo, fence ? fence->m_fence : VK_NULL_HANDLE));
    }

    void Device::SubmitCommandBuffers(CommandBufferSubmitInfo submitInfo) const
    {
        if (submitInfo.commandBuffers.empty()) {
            LogError("No command buffers provided for submission.");
            return;
        }

        std::vector<VkCommandBuffer> rawCommandBuffers;
        rawCommandBuffers.reserve(submitInfo.commandBuffers.size());
        for (const auto* cmdBuffer : submitInfo.commandBuffers) {
            if (cmdBuffer && cmdBuffer->GetRaw() != VK_NULL_HANDLE) {
                rawCommandBuffers.push_back(cmdBuffer->GetRaw());
            } else {
                LogError("Invalid command buffer in submission list.");
                return;
            }
        }

        std::vector<VkSemaphore> waitSemaphores;
        for (const auto* semaphore : submitInfo.waitSemaphores) {
            if (semaphore && semaphore->GetRaw() != VK_NULL_HANDLE) {
                waitSemaphores.push_back(semaphore->GetRaw());
            } else {
                LogError("Invalid wait semaphore in submission list.");
                return;
            }
        }

        std::vector<VkSemaphore> signalSemaphores;
        for (const auto* semaphore : submitInfo.signalSemaphores) {
            if (semaphore && semaphore->GetRaw() != VK_NULL_HANDLE) {
                signalSemaphores.push_back(semaphore->GetRaw());
            } else {
                LogError("Invalid signal semaphore in submission list.");
                return;
            }
        }

        VkPipelineStageFlags waitStageMask = static_cast<VkPipelineStageFlags>(submitInfo.waitStageMask);
        VkSubmitInfo vkSubmitInfo{
            .sType = VK_STRUCTURE_TYPE_SUBMIT_INFO,
            .waitSemaphoreCount = static_cast<uint32_t>(waitSemaphores.size()),
            .pWaitSemaphores = waitSemaphores.data(),
            .pWaitDstStageMask = &waitStageMask,
            .commandBufferCount = static_cast<uint32_t>(rawCommandBuffers.size()),
            .pCommandBuffers = rawCommandBuffers.data(),
            .signalSemaphoreCount = static_cast<uint32_t>(signalSemaphores.size()),
            .pSignalSemaphores = signalSemaphores.data()
        };

        CheckVk(vkQueueSubmit(GetGraphicsQueue().GetRaw(), 1, &vkSubmitInfo, submitInfo.fence ? submitInfo.fence->GetRaw() : VK_NULL_HANDLE));
    }

    void Device::WaitForFence(const Fence& fence, uint64_t timeout) const
    {
        if (fence.m_fence == VK_NULL_HANDLE) {
            LogError("Invalid fence.");
            return;
        }
        CheckVk(vkWaitForFences(m_device, 1, &fence.m_fence, VK_TRUE, timeout));
    }

    void Device::ResetFence(const Fence &fence) const
    {
        if (fence.m_fence == VK_NULL_HANDLE) {
            LogError("Invalid fence.");
            return;
        }
        CheckVk(vkResetFences(m_device, 1, &fence.m_fence));
    }

    void Device::WaitIdle() const
    {
        CheckVk(vkDeviceWaitIdle(m_device));
    }

    void Device::Present(const Swapchain &swapchain, uint32_t imageIndex, const std::vector<const Semaphore *> &waitSemaphores) const
    {
        std::vector<VkSemaphore> rawWaitSemaphores;
        rawWaitSemaphores.reserve(waitSemaphores.size());
        for (const auto* semaphore : waitSemaphores) {
            if (semaphore && semaphore->GetRaw() != VK_NULL_HANDLE) {
                rawWaitSemaphores.push_back(semaphore->GetRaw());
            } else {
                LogError("Invalid wait semaphore in present list.");
                return;
            }
        }

        VkSwapchainKHR rawSwapchain = swapchain.GetRaw();
        VkPresentInfoKHR presentInfo{
            .sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR,
            .waitSemaphoreCount = static_cast<uint32_t>(rawWaitSemaphores.size()),
            .pWaitSemaphores = rawWaitSemaphores.data(),
            .swapchainCount = 1,
            .pSwapchains = &rawSwapchain,
            .pImageIndices = &imageIndex
        };

        VkResult result = vkQueuePresentKHR(GetGraphicsQueue().GetRaw(), &presentInfo);
        if (result != VK_SUCCESS) {
            LogError("Failed to present swapchain image.");
        }
    }

    Texture Device::CreateKTXTextureFromFile(const std::string &filePath, const CommandPool &commandPool)
    {
        ktxTexture* ktxTexture = nullptr;
        ktxTexture_CreateFromNamedFile(filePath.c_str(), KTX_TEXTURE_CREATE_LOAD_IMAGE_DATA_BIT, &ktxTexture);

        if (!ktxTexture) {
            LogError("Failed to load texture from file: {}", filePath);
            return Texture();
        }

        Texture texture;
        
        Image img = CreateImage({
            .format = static_cast<ImageFormat>(ktxTexture_GetVkFormat(ktxTexture)),
            .extent = { ktxTexture->baseWidth, ktxTexture->baseHeight, ktxTexture->baseDepth },
            .mipLevels = ktxTexture->numLevels,
            .usage = ImageUsageFlag::Sampled | ImageUsageFlag::TransferDst
        });
        
        // temp buffer for uploading texture data
        Buffer stagingBuffer = CreateBuffer(
            ktxTexture->dataSize, 
            BufferUsageFlag::TransferSrc, 
            AllocationFlag::HostAccessSequentialWrite | AllocationFlag::Mapped);

        memcpy(stagingBuffer.GetMappedData(), ktxTexture->pData, ktxTexture->dataSize);

        Fence fence = CreateFence();
        CommandBuffer commandBuffer = CreateCommandBuffer(commandPool);

        commandBuffer.Begin();
        
        commandBuffer.TransitionImage({
            .image = &img,
            .oldLayout = ImageLayout::Undefined,
            .newLayout = ImageLayout::TransferDst,
            .dst = SyncScope::TransferWrite(),
            .range = ImageSubresourceRange::Color(ktxTexture->numLevels),
        });

        std::vector<BufferToImageCopyRegion> copyRegions{};
        copyRegions.reserve(ktxTexture->numLevels);
        for (auto j = 0; j < ktxTexture->numLevels; j++) {
            ktx_size_t mipOffset{0};
            KTX_error_code ret = ktxTexture_GetImageOffset(ktxTexture, j, 0, 0, &mipOffset);
            if (ret != KTX_SUCCESS) {
                LogError("Failed to obtain mip {} offset for texture: {}", j, filePath);
                ktxTexture_Destroy(ktxTexture);
                return Texture();
            }
            copyRegions.push_back({
                .bufferOffset = mipOffset,
                .mipLevel = static_cast<uint32_t>(j),
                .extent = { .width = ktxTexture->baseWidth >> j, .height = ktxTexture->baseHeight >> j, .depth = 1 },
            });
        }
        commandBuffer.CopyBufferToImage(stagingBuffer, img, ImageLayout::TransferDst, copyRegions);

        commandBuffer.TransitionImage({
            .image = &img,
            .oldLayout = ImageLayout::TransferDst,
            .newLayout = ImageLayout::ReadOnly,
            .src = SyncScope::TransferWrite(),
            .dst = SyncScope::FragmentShaderRead(),
            .range = ImageSubresourceRange::Color(ktxTexture->numLevels),
        });

        commandBuffer.End();
        SubmitCommandBuffer(commandBuffer, &fence);
        WaitForFence(fence);
        
        ktxTexture_Destroy(ktxTexture);

        texture.m_image = std::move(img);

        texture.m_device = this;
        texture.isArray = ktxTexture->isArray;
        texture.isCubemap = ktxTexture->isCubemap;
        texture.isCompressed = ktxTexture->isCompressed;
        texture.generateMipmaps = ktxTexture->generateMipmaps;
        texture.baseWidth = ktxTexture->baseWidth;
        texture.baseHeight = ktxTexture->baseHeight;
        texture.baseDepth = ktxTexture->baseDepth;
        texture.numDimensions = ktxTexture->numDimensions;
        texture.numLevels = ktxTexture->numLevels;
        texture.numLayers = ktxTexture->numLayers;
        texture.numFaces = ktxTexture->numFaces;

        return texture;
    }

    BindlessTextureDescriptors Device::CreateBindlessTextureDescriptors(const std::vector<TextureDescriptor> &textureDescriptors) const
    {
        return BindlessTextureDescriptors(*this, textureDescriptors);
    }

    GraphicsPipelineBuilder Device::CreateGraphicsPipelineBuilder() const
    {
        return GraphicsPipelineBuilder(*this);
    }
}
