#include <iostream>
#include <spdlog/spdlog.h>

#include "nova.h"
#include "nv_window.h"
#include "nv_swapchain.h"

int main() {
    Nova::Instance instance;
    Nova::Window window(instance, 800, 600, "Nova Engine");
    Nova::Device device(&instance, &window);

    Nova::SwapchainBuilder swapchainBuilder(&device);
    Nova::Swapchain swapchain = swapchainBuilder
        .SetFormat(Nova::ImageFormat::B8G8R8A8Unorm)
        .SetColorSpace(Nova::ColorSpace::SrgbNonlinear)
        .SetPresentMode(Nova::SwapchainPresentMode::Fifo)
        .SetExtent(window.GetSize().x, window.GetSize().y)
        .build();
    
    return 0;
}
