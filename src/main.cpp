#include <iostream>
#include <spdlog/spdlog.h>

#include "nova.h"

int main() {
    Nova::Instance instance;
    Nova::Window window(instance, 800, 600, "Nova Engine");
    Nova::Device device(instance);

    Nova::Swapchain swapchain = device.CreateSwapchainBuilder(window.GetSurface())
        .SetFormat(Nova::ImageFormat::B8G8R8A8Unorm)
        .SetColorSpace(Nova::ColorSpace::SrgbNonlinear)
        .SetPresentMode(Nova::SwapchainPresentMode::Fifo)
        .SetExtent(window.GetWidth(), window.GetHeight())
        .build();

    Nova::Image depthImage = device.CreateImageBuilder()
        .SetFormat(device.GetSupportedDepthFormats()[0])
        .SetExtent(window.GetWidth(), window.GetHeight())
        .SetUsage(Nova::ImageUsageFlag::DepthStencilAttachment)
        .build(instance, Nova::AllocationFlag::DedicatedMemory);

    Nova::ImageView depthImageView = depthImage.CreateView(Nova::ImageAspectFlag::Depth);

    Nova::AllocationFlag allocationFlags = Nova::AllocationFlag::HostAccessSequentialWrite | Nova::AllocationFlag::HostAccessAllowTransfer | Nova::AllocationFlag::Mapped;
    Nova::Buffer b1 = device.CreateBuffer(1024, Nova::BufferUsageFlag::VertexBuffer, allocationFlags);
    Nova::Buffer b2 = device.CreateBuffer(2048, Nova::BufferUsageFlag::IndexBuffer, allocationFlags);
    Nova::Buffer b3 = device.CreateBuffer(4096, Nova::BufferUsageFlag::ShaderDeviceAddress, allocationFlags);
    printf("Buffer device address: 0x%llx\n", (unsigned long long)b3.GetBufferDeviceAddress());

    Nova::Fence fence = device.CreateFence(true);
    Nova::Semaphore semaphore = device.CreateSemaphore();

    Nova::CommandPool commandPool = device.CreateCommandPool();
    Nova::CommandBuffer commandBuffer = device.CreateCommandBuffer(commandPool);

    Nova::Texture texture = Nova::Texture::CreateFromFile("res/test1.ktx", device, commandPool);
    return 0;
}
