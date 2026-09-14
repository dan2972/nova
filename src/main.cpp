#include <iostream>
#include <spdlog/spdlog.h>

#include "nova.h"

int main() {
    Nova::Instance instance;
    Nova::Window window(instance, 800, 600, "Nova Engine");
    Nova::Device device(&instance, &window);

    Nova::SwapchainBuilder swapchainBuilder(&device);
    Nova::Swapchain swapchain = swapchainBuilder
        .SetFormat(Nova::ImageFormat::B8G8R8A8Unorm)
        .SetColorSpace(Nova::ColorSpace::SrgbNonlinear)
        .SetPresentMode(Nova::SwapchainPresentMode::Fifo)
        .SetExtent(window.GetWidth(), window.GetHeight())
        .build();

    Nova::ImageBuilder imageBuilder(&device);
    Nova::Image depthImage = imageBuilder
        .SetFormat(device.GetSupportedDepthFormats()[0])
        .SetExtent(window.GetWidth(), window.GetHeight())
        .SetUsage(Nova::ImageUsageFlag::DepthStencilAttachment)
        .build(instance);

    Nova::Buffer b1 = device.CreateBuffer(1024, Nova::BufferUsageFlag::VertexBuffer);
    Nova::Buffer b2 = device.CreateBuffer(2048, Nova::BufferUsageFlag::IndexBuffer);
    Nova::Buffer b3 = device.CreateBuffer(4096, Nova::BufferUsageFlag::ShaderDeviceAddress);
    printf("Buffer device address: 0x%llx\n", (unsigned long long)b3.GetBufferDeviceAddress());

    Nova::Fence fence = device.CreateFence();
    Nova::Semaphore semaphore = device.CreateSemaphore();
    return 0;
}
