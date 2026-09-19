
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
            .queueFamilyIndex = queueFamilyIndex,
            .flags = static_cast<VkCommandPoolCreateFlags>(flags)
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
        CheckVk(vkQueueSubmit(graphicsQueue, 1, &submitInfo, fence ? fence->m_fence : VK_NULL_HANDLE));
    }

    void Device::WaitForFence(Fence* fence, uint64_t timeout) const
    {
        if (fence == nullptr || fence->m_fence == VK_NULL_HANDLE) {
            LogError("Invalid fence.");
            return;
        }
        CheckVk(vkWaitForFences(m_device, 1, &fence->m_fence, VK_TRUE, timeout));
    }

    Texture Device::CreateFromFile(const std::string &filePath, const CommandPool &commandPool)
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
            .usage = ImageUsageFlag::Sampled | ImageUsageFlag::TransferDst,
            .mipLevels = ktxTexture->numLevels
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
        
        commandBuffer.TransitionImage(img, {
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

        commandBuffer.TransitionImage(img, {
            .oldLayout = ImageLayout::TransferDst,
            .newLayout = ImageLayout::ReadOnly,
            .src = SyncScope::TransferWrite(),
            .dst = SyncScope::FragmentShaderRead(),
            .range = ImageSubresourceRange::Color(ktxTexture->numLevels),
        });

        commandBuffer.End();
        SubmitCommandBuffer(&commandBuffer, &fence);
        WaitForFence(&fence);
        
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
}
