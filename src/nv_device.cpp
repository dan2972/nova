
#include <algorithm>
#include "nv_helpers.h"
#include "nv_device.h"
#include "nv_logger.h"

namespace {
    void sGetPhysicalDevices(VkInstance instance, std::vector<Nova::PhysicalDevice>& physicalDevices)
    {
        uint32_t deviceCount = 0;
        std::vector<VkPhysicalDevice> physicalDevicesRaw;
        CheckVk(vkEnumeratePhysicalDevices(instance, &deviceCount, nullptr));
        physicalDevicesRaw.resize(deviceCount);
        CheckVk(vkEnumeratePhysicalDevices(instance, &deviceCount, physicalDevicesRaw.data()));
        physicalDevices.reserve(deviceCount);
        for (uint32_t i = 0; i < deviceCount; ++i) {
            physicalDevices.emplace_back(physicalDevicesRaw[i]);
        }
    }

    // sort by device type, preferring discrete GPUs,
    // then by amount of VRAM, preferring more VRAM
    void sSortPhysicalDevices(std::vector<Nova::PhysicalDevice>& physicalDevices)
    {
        std::sort(physicalDevices.begin(), physicalDevices.end(),
            [](Nova::PhysicalDevice a, Nova::PhysicalDevice b) {
                if (a.GetType() != b.GetType()) {
                    return a.GetType() == Nova::PhysicalDevice::GPUType::DiscreteGPU;
                }

                return a.GetVRAMSize() > b.GetVRAMSize();
            });
    }

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
    Device::Device(Instance* instance, VkSurfaceKHR surface)
        : m_instance(instance), m_surface(surface)
    {
        if (!m_instance) {
            LogError("Instance is null.");
            exit(-1);
        }

        sGetPhysicalDevices(m_instance->m_instance, m_physicalDevices);
        sSortPhysicalDevices(m_physicalDevices);

        if (m_physicalDevices.empty()) {
            LogError("No GPU found.");
            exit(-1);
        }

        m_selectedPhysicalDeviceIndex = 0;
        
        const PhysicalDevice& selectedDevice = m_physicalDevices[m_selectedPhysicalDeviceIndex];
        LogInfo("Selected GPU: {} (type: {}, VRAM: {} MB)",
            selectedDevice.GetDeviceName(),
            static_cast<int>(selectedDevice.GetType()),
            selectedDevice.GetVRAMSize() / (1024 * 1024));

        bool success = sCreateLogicalDevice(selectedDevice, selectedDevice.GetGraphicsQueueFamilyIndex(), m_device);
        if (!success) {
            LogError("Failed to create logical device.");
        }
    }

    Device::Device(Instance* instance, Window* window)
        : Device(instance, window->GetSurface())
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

    void Device::SelectPhysicalDevice(size_t index)  {
        if (index >= m_physicalDevices.size()) {
            LogError("Invalid physical device index: {}", index);
            exit(-1);
        }
        m_selectedPhysicalDeviceIndex = index;
    }

    const VkSurfaceCapabilitiesKHR Device::GetSurfaceCapabilities() const {
        if (m_surface == VK_NULL_HANDLE) {
            LogError("Surface is not set for the device.");
            exit(-1);
        }
        VkSurfaceCapabilitiesKHR capabilities;
        vkGetPhysicalDeviceSurfaceCapabilitiesKHR(GetPhysicalDevice().GetRaw(), m_surface, &capabilities);
        return capabilities;
    }

    std::vector<ImageFormat> Device::GetSupportedDepthFormats() const
    {
        static const std::vector<VkFormat> depthFormats = {
            VK_FORMAT_D32_SFLOAT_S8_UINT,
            VK_FORMAT_D24_UNORM_S8_UINT
        };

        std::vector<ImageFormat> supportedDepthFormats;
        for (const auto& format : depthFormats) {
            VkFormatProperties2 formatProperties{ .sType = VK_STRUCTURE_TYPE_FORMAT_PROPERTIES_2 };
            vkGetPhysicalDeviceFormatProperties2(GetPhysicalDevice().GetRaw(), format, &formatProperties);
            if (formatProperties.formatProperties.optimalTilingFeatures & VK_FORMAT_FEATURE_DEPTH_STENCIL_ATTACHMENT_BIT) {
                supportedDepthFormats.push_back(static_cast<ImageFormat>(format));
            }
        }
        return supportedDepthFormats;
    }

    Buffer Device::CreateBuffer(BufferSize size, BufferUsageFlag usageFlags, AllocationFlag allocationFlags)
    {
        VkBufferCreateInfo bufferCreateInfo{
            .sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO,
            .size = size,
            .usage = static_cast<VkBufferUsageFlags>(usageFlags)
        };

        BufferAllocation allocation = CreateBufferAllocation(*m_instance, *this, bufferCreateInfo, static_cast<AllocationFlag>(allocationFlags));

        Buffer buffer{};
        buffer.allocation = allocation;
        buffer.size = size;
        buffer.usageFlags = usageFlags;
        buffer.device = m_device;

        if ((usageFlags & BufferUsageFlag::ShaderDeviceAddress) != BufferUsageFlag::None) {
            VkBufferDeviceAddressInfo bufferDeviceAddressInfo{
                .sType = VK_STRUCTURE_TYPE_BUFFER_DEVICE_ADDRESS_INFO,
                .buffer = allocation.buffer
            };
            VkDeviceAddress address = vkGetBufferDeviceAddress(m_device, &bufferDeviceAddressInfo);
            buffer.deviceAddress = static_cast<BufferDeviceAddress>(address);
        }

        return buffer;
    }

    Fence Device::CreateFence(bool signaled)
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

    Semaphore Device::CreateSemaphore()
    {
        static const VkSemaphoreCreateInfo semaphoreCreateInfo{
            .sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO
        };

        Semaphore semaphore{};
        semaphore.device = m_device;
        CheckVk(vkCreateSemaphore(m_device, &semaphoreCreateInfo, nullptr, &semaphore.semaphore));

        return semaphore;
    }

    CommandPool Device::CreateCommandPool(CommandPoolCreateFlag flags)
    {
        return CreateCommandPool(GetPhysicalDevice().GetGraphicsQueueFamilyIndex(), flags);
    }

    CommandPool Device::CreateCommandPool(uint32_t queueFamilyIndex, CommandPoolCreateFlag flags)
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

    CommandBuffer Device::CreateCommandBuffer(CommandPool& commandPool)
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
}
