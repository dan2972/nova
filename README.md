# Nova

***\*This project is still WIP!\****

A small wrapper/abstraction of "modern" (1.3+) Vulkan I made for kickstarting personal projects. This library only supports dynamic rendering and bindless rendering using descriptor indexing. Shader buffers are also accessed through buffer device addresses.

## Dependencies
This project uses C++20 with the following dependencies managed throuch vcpkg:
- glm
- volk
- vulkan-memory-allocator
- shader-slang
- spdlog
- ktx
- sdl3

## Example Usage
*see main.cpp*
```cpp
#include "nova.h"

int main() {
    nova::Instance instance;
    nova::Window window(instance, 800, 600, "Nova Engine");
    nova::Device device(instance);

    nova::Swapchain swapchain = device.CreateSwapchainBuilder(window.GetSurface())
        .SetDesiredFormat(nova::ImageFormat::B8G8R8A8Unorm)
        .SetDesiredColorSpace(nova::ColorSpace::SrgbNonlinear)
        .SetDesiredPresentMode(nova::SwapchainPresentMode::Fifo)
        .SetDesiredExtent(window.GetFramebufferSize())
        .build();

    nova::Image depthImage = device.CreateImage({
        .format = device.GetSupportedDepthFormats()[0],
        .extent = { window.GetFramebufferWidth(), window.GetFramebufferHeight(), 1 },
        .usage = nova::ImageUsageFlag::DepthStencilAttachment
    });

    nova::ImageView depthImageView = depthImage.CreateView(nova::ImageAspectFlag::Depth);

    std::vector<Vertex> vertices{};
    std::vector<uint16_t> indices{};

    nova::AllocationFlag allocationFlags = 
        nova::AllocationFlag::HostAccessSequentialWrite |
        nova::AllocationFlag::Mapped;
    size_t vertexBufferSize = vertices.size() * sizeof(Vertex);
    size_t indexBufferSize = indices.size() * sizeof(uint16_t);
    size_t bufferSize = vertexBufferSize + indexBufferSize;
    nova::Buffer buffer = device.CreateBuffer(bufferSize, nova::BufferUsageFlag::VertexBuffer | nova::BufferUsageFlag::IndexBuffer, allocationFlags);
    memcpy(buffer.GetMappedData(), vertices.data(), vertices.size() * sizeof(Vertex));
    memcpy(static_cast<uint8_t*>(buffer.GetMappedData()) + vertexBufferSize, indices.data(), indices.size() * sizeof(uint16_t));

    nova::Fence fence = device.CreateFence(true);
    nova::Semaphore semaphore = device.CreateSemaphore();

    nova::CommandPool commandPool = device.CreateCommandPool();
    nova::CommandBuffer commandBuffer = device.CreateCommandBuffer(commandPool);

    nova::Texture texture = device.CreateKTXTextureFromFile("res/test1.ktx", commandPool);
    nova::Sampler sampler = texture.CreateLinearSampler();
    nova::ImageView textureView = texture.CreateView();

    nova::BindlessTextureDescriptors bindlessTextureDescriptors = device.CreateBindlessTextureDescriptors({
        { &textureView, &sampler }
    });

    nova::ShaderCompiler shaderCompiler;
    shaderCompiler.BeginSession();
    nova::SPIRVData spirv = shaderCompiler.CompileToSPIRVFromShaderPath("res/test1.slang");
    nova::ShaderModule shaderModule = shaderCompiler.CompileToShaderModuleFromShaderPath("res/test1.slang", &device);
    shaderCompiler.EndSession();

    nova::GraphicsPipelineBuilder pipelineBuilder = device.CreateGraphicsPipelineBuilder();
    pipelineBuilder
        .AddDescriptorSetLayout(bindlessTextureDescriptors.GetDescriptorSetLayout())
        .AddPushConstantRange({
            .stageFlags = nova::ShaderStageFlag::Vertex,
            .offset = 0,
            .size = sizeof(double)
        })
        .AddVertexBindingLayout({
            .binding = 0,
            .stride = sizeof(float) * 8,
            .attributes = {
                { .format = nova::VertexFormat::Float3, .offset = 0 },
                { .format = nova::VertexFormat::Float3, .offset = sizeof(float) * 3 },
                { .format = nova::VertexFormat::Float2, .offset = sizeof(float) * 6 }
            }
        })
        .AddShaderStage(nova::ShaderStageFlag::Vertex, shaderModule, "main")
        .AddShaderStage(nova::ShaderStageFlag::Fragment, shaderModule, "main")
        .EnableDepthStencil(true, device.GetSupportedDepthFormats()[0])
        .SetColorFormat(nova::ImageFormat::B8G8R8A8Unorm);

    nova::GraphicsPipeline pipeline = pipelineBuilder.Build();
    return 0;
}
```