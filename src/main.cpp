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
        .SetUsage(Nova::ImageUsage::DepthStencilAttachment)
        .build(instance);
    
    return 0;
}
