#include <iostream>
#include <spdlog/spdlog.h>

#include "nova.h"

int main() {
    Nova::Instance instance;
    Nova::Window window(instance, 800, 600, "Nova Engine");
    Nova::Device device(instance);

    Nova::Swapchain swapchain = device.CreateSwapchainBuilder(window.GetSurface())
        .SetDesiredFormat(Nova::ImageFormat::B8G8R8A8Unorm)
        .SetDesiredColorSpace(Nova::ColorSpace::SrgbNonlinear)
        .SetDesiredPresentMode(Nova::SwapchainPresentMode::Fifo)
        .SetDesiredExtent(window.GetSize())
        .build();

    Nova::Image depthImage = device.CreateImage({
        .format = device.GetSupportedDepthFormats()[0],
        .extent = { window.GetWidth(), window.GetHeight(), 1 },
        .usage = Nova::ImageUsageFlag::DepthStencilAttachment
    });

    Nova::ImageView depthImageView = depthImage.CreateView(Nova::ImageAspectFlag::Depth);

    Nova::AllocationFlag allocationFlags = 
        Nova::AllocationFlag::HostAccessSequentialWrite | 
        Nova::AllocationFlag::HostAccessAllowTransfer | 
        Nova::AllocationFlag::Mapped;
    Nova::Buffer b1 = device.CreateBuffer(1024, Nova::BufferUsageFlag::VertexBuffer, allocationFlags);
    Nova::Buffer b2 = device.CreateBuffer(2048, Nova::BufferUsageFlag::IndexBuffer, allocationFlags);
    Nova::Buffer b3 = device.CreateBuffer(4096, Nova::BufferUsageFlag::ShaderDeviceAddress, allocationFlags);
    printf("Buffer device address: 0x%llx\n", (unsigned long long)b3.GetBufferDeviceAddress());

    Nova::Fence fence = device.CreateFence(true);
    Nova::Semaphore semaphore = device.CreateSemaphore();

    Nova::CommandPool commandPool = device.CreateCommandPool();
    Nova::CommandBuffer commandBuffer = device.CreateCommandBuffer(commandPool);

    Nova::Texture texture = device.CreateKTXTextureFromFile("res/test1.ktx", commandPool);
    Nova::Sampler sampler = texture.CreateLinearSampler();
    Nova::ImageView textureView = texture.CreateView();

    Nova::BindlessTextureDescriptors bindlessTextureDescriptors = device.CreateBindlessTextureDescriptors({
        { &textureView, &sampler }
    });

    Nova::ShaderCompiler shaderCompiler;
    shaderCompiler.BeginSession();
    Nova::SPIRVData spirv = shaderCompiler.CompileToSPIRVFromShaderPath("res/test1.slang");
    Nova::ShaderModule shaderModule = shaderCompiler.CompileToShaderModuleFromShaderPath("res/test1.slang", &device);
    shaderCompiler.EndSession();

    Nova::ShaderCompiler::WriteSPIRVToDisk(spirv, "res/test1.spirv");
    Nova::SPIRVData spirvFromDisk = Nova::ShaderCompiler::ReadSPIRVFromDisk("res/test1.spirv");

    Nova::GraphicsPipelineBuilder pipelineBuilder(device);
    pipelineBuilder
        .AddDescriptorSetLayout(bindlessTextureDescriptors.GetDescriptorSetLayout())
        .AddPushConstantRange({
            .stageFlags = Nova::ShaderStageFlag::Vertex,
            .offset = 0,
            .size = sizeof(double)
        })
        .AddVertexBindingLayout({
            .binding = 0,
            .stride = sizeof(float) * 8,
            .attributes = {
                { .format = Nova::VertexFormat::Float3, .offset = 0 },
                { .format = Nova::VertexFormat::Float3, .offset = sizeof(float) * 3 },
                { .format = Nova::VertexFormat::Float2, .offset = sizeof(float) * 6 }
            }
        })
        .AddShaderStage(Nova::ShaderStageFlag::Vertex, shaderModule, "main")
        .AddShaderStage(Nova::ShaderStageFlag::Fragment, shaderModule, "main")
        .EnableDepthStencil(true, device.GetSupportedDepthFormats()[0])
        .SetColorFormat(Nova::ImageFormat::B8G8R8A8Unorm);

    Nova::GraphicsPipeline pipeline = pipelineBuilder.Build();
    
    return 0;
}
