#include <iostream>
#include <spdlog/spdlog.h>

#include "nova.h"

#include <tiny_obj_loader.h>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/quaternion.hpp>
#include <glm/glm.hpp>

struct Vertex {
    glm::vec3 pos;
    glm::vec3 normal;
    glm::vec2 uv;
};

struct ShaderData {
	glm::mat4 projection;
	glm::mat4 view;
	glm::mat4 model[3];
	glm::vec4 lightPos = { 0.0f, -10.0f, 10.0f, 0.0f };
	uint32_t selected = 1;
};

static constexpr uint32_t maxFramesInFlight = 2;

bool updateSwapchain = false;

int main() {
    Nova::Instance instance;
    Nova::Window window(instance, 800, 600, "Nova Engine");
    Nova::Device device(instance);

    Nova::FramebufferResizeCallback framebufferResizeCallback = [](Nova::Window* window, int width, int height) {
        updateSwapchain = true;
    };
    window.SetFramebufferResizeCallback(framebufferResizeCallback);

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

    tinyobj::attrib_t attrib;
    std::vector<tinyobj::shape_t> shapes;
    std::vector<tinyobj::material_t> materials;
    tinyobj::LoadObj(&attrib, &shapes, &materials, nullptr, nullptr, "res/suzanne.obj");
    const VkDeviceSize indexCount{shapes[0].mesh.indices.size()};   

    std::vector<Vertex> vertices{};
    std::vector<uint16_t> indices{};

    for (auto& index : shapes[0].mesh.indices) {
        Vertex v{
            .pos = { attrib.vertices[index.vertex_index * 3], -attrib.vertices[index.vertex_index * 3 + 1], attrib.vertices[index.vertex_index * 3 + 2] },
            .normal = { attrib.normals[index.normal_index * 3], -attrib.normals[index.normal_index * 3 + 1], attrib.normals[index.normal_index * 3 + 2] },
            .uv = { attrib.texcoords[index.texcoord_index * 2], 1.0 - attrib.texcoords[index.texcoord_index * 2 + 1] }
        };
        vertices.push_back(v);
        indices.push_back(indices.size());
    }

    Nova::AllocationFlag allocationFlags = 
        Nova::AllocationFlag::HostAccessSequentialWrite | 
        Nova::AllocationFlag::HostAccessAllowTransfer | 
        Nova::AllocationFlag::Mapped;
    Nova::Buffer vertexBuffer = device.CreateBuffer(vertices.size() * sizeof(Vertex), Nova::BufferUsageFlag::VertexBuffer, allocationFlags);
    Nova::Buffer indexBuffer = device.CreateBuffer(indices.size() * sizeof(uint16_t), Nova::BufferUsageFlag::IndexBuffer, allocationFlags);
    memcpy(vertexBuffer.GetMappedData(), vertices.data(), vertices.size() * sizeof(Vertex));
    memcpy(indexBuffer.GetMappedData(), indices.data(), indices.size() * sizeof(uint16_t));

    Nova::CommandPool commandPool = device.CreateCommandPool(Nova::CommandPoolCreateFlag::ResetCommandBuffer);

    std::array<Nova::Buffer, maxFramesInFlight> shaderDataBuffers;
    std::array<Nova::CommandBuffer, maxFramesInFlight> commandBuffers;
    std::array<Nova::Fence, maxFramesInFlight> fences;
    std::array<Nova::Semaphore, maxFramesInFlight> renderSemaphores;
    std::vector<Nova::Semaphore> presentSemaphores;

    for (size_t i = 0; i < maxFramesInFlight; ++i) {
        shaderDataBuffers[i] = device.CreateBuffer(sizeof(ShaderData), Nova::BufferUsageFlag::ShaderDeviceAddress, allocationFlags);
        commandBuffers[i] = device.CreateCommandBuffer(commandPool);

        renderSemaphores[i] = device.CreateSemaphore();
        fences[i] = device.CreateFence(true);
    }

    presentSemaphores.resize(swapchain.GetImages().size());
    for (size_t i = 0; i < presentSemaphores.size(); ++i) {
        presentSemaphores[i] = device.CreateSemaphore();
    }

    std::array<Nova::Texture, 3> textures;
    std::array<Nova::Sampler, 3> samplers;
    std::array<Nova::ImageView, 3> textureViews;
    for (size_t i = 0; i < textures.size(); ++i) {
        std::string texturePath = "res/suzanne" + std::to_string(i) + ".ktx";
        textures[i] = device.CreateKTXTextureFromFile(texturePath, commandPool);
        samplers[i] = textures[i].CreateLinearSampler();
        textureViews[i] = textures[i].CreateView();
    }

    Nova::BindlessTextureDescriptors bindlessTextureDescriptors = device.CreateBindlessTextureDescriptors({
        { &textureViews[0], &samplers[0] },
        { &textureViews[1], &samplers[1] },
        { &textureViews[2], &samplers[2] }
    });

    Nova::ShaderCompiler shaderCompiler;
    shaderCompiler.BeginSession();
    Nova::SPIRVData spirv = shaderCompiler.CompileToSPIRVFromShaderPath("res/test1.slang");
    Nova::ShaderModule shaderModule = shaderCompiler.CompileToShaderModuleFromShaderPath("res/test1.slang", &device);
    shaderCompiler.EndSession();

    // Nova::ShaderCompiler::WriteSPIRVToDisk(spirv, "res/test1.spirv");
    // Nova::SPIRVData spirvFromDisk = Nova::ShaderCompiler::ReadSPIRVFromDisk("res/test1.spirv");

    Nova::GraphicsPipelineBuilder pipelineBuilder = device.CreateGraphicsPipelineBuilder();
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

    bool quit = false;
    uint64_t lastTime = window.GetTimeSeconds();
    uint32_t frameIndex = 0;
    uint32_t imageIndex = 0;
    // bool updateSwapchain = false;
    ShaderData shaderData{};
    glm::vec3 camPos = { 0.0f, 0.0f, -6.0f };
    glm::vec3 objectRotations[3]{};
    while (!quit) {
        device.WaitForFence(fences[frameIndex]);
        device.ResetFence(fences[frameIndex]);

        const Nova::Image swapchainImage = swapchain.AcquireNextImage(imageIndex, &renderSemaphores[frameIndex]);

        Nova::Extent2D windowSize = window.GetSize();
        shaderData.projection = glm::perspective(glm::radians(45.0f), (float)windowSize.width / (float)windowSize.height, 0.1f, 32.0f);
		shaderData.view = glm::translate(glm::mat4(1.0f), camPos);
		for (auto i = 0; i < 3; i++) {
			auto instancePos = glm::vec3((float)(i - 1) * 3.0f, 0.0f, 0.0f);
			shaderData.model[i] = glm::translate(glm::mat4(1.0f), instancePos) * glm::mat4_cast(glm::quat(objectRotations[i]));
		}
		memcpy(shaderDataBuffers[frameIndex].GetMappedData(), &shaderData, sizeof(ShaderData));

        Nova::CommandBuffer& commandBuffer = commandBuffers[frameIndex];
        commandBuffer.Reset();
        commandBuffer.Begin();

        commandBuffer.TransitionImages({
            Nova::ImageTransition{
                .image = &swapchainImage,
                .oldLayout = Nova::ImageLayout::Undefined,
                .newLayout = Nova::ImageLayout::AttachmentOptimal,
                .src = {Nova::PipelineStageFlag::ColorAttachmentOutput, Nova::AccessFlag::None},
                .dst = {Nova::PipelineStageFlag::ColorAttachmentOutput, Nova::AccessFlag::ColorAttachmentRead | Nova::AccessFlag::ColorAttachmentWrite},
                .range = Nova::ImageSubresourceRange::Color(),
            },
            Nova::ImageTransition{
                .image = &depthImage,
                .oldLayout = Nova::ImageLayout::Undefined,
                .newLayout = Nova::ImageLayout::AttachmentOptimal,
                .src = {Nova::PipelineStageFlag::LateFragmentTests, Nova::AccessFlag::DepthStencilAttachmentWrite},
                .dst = {Nova::PipelineStageFlag::EarlyFragmentTests, Nova::AccessFlag::DepthStencilAttachmentWrite},
                .range = Nova::ImageSubresourceRange::DepthStencil(),
            }
        });

        commandBuffer.BeginRendering({
            Nova::RenderAttachmentInfo::ColorAttachment(swapchain.GetImageViews()[imageIndex], {0.0f, 0.0f, 0.0f, 1.0f}),
            Nova::RenderAttachmentInfo::DepthAttachment(depthImageView, {1.0f, 0.0f, 0.0f, 0.0f})
        }, windowSize);

        commandBuffer.SetViewport({0.0f, 0.0f, (float)windowSize.width, (float)windowSize.height, 0.0f, 1.0f});
        commandBuffer.BindGraphicsPipeline(pipeline);
        commandBuffer.SetScissor({0, 0, windowSize.width, windowSize.height});
        commandBuffer.BindDescriptorSet(pipeline.GetPipelineLayout(), bindlessTextureDescriptors.GetDescriptorSet());
        commandBuffer.BindVertexBuffer(vertexBuffer);
        commandBuffer.BindIndexBuffer(indexBuffer, 0, Nova::IndexType::Uint16);
        auto shaderDataBufferDeviceAddress = shaderDataBuffers[frameIndex].GetBufferDeviceAddress();
        commandBuffer.PushConstants(pipeline.GetPipelineLayout(), Nova::ShaderStageFlag::Vertex, 0, sizeof(double), &shaderDataBufferDeviceAddress);
        commandBuffer.DrawIndexed(indexCount, 3);
        commandBuffer.EndRendering();
        commandBuffer.TransitionImage({
            .image = &swapchainImage,
            .oldLayout = Nova::ImageLayout::AttachmentOptimal,
            .newLayout = Nova::ImageLayout::PresentSrc,
            .src = {Nova::PipelineStageFlag::ColorAttachmentOutput, Nova::AccessFlag::ColorAttachmentWrite},
            .dst = {Nova::PipelineStageFlag::ColorAttachmentOutput, Nova::AccessFlag::None},
            .range = Nova::ImageSubresourceRange::Color(),
        });
        commandBuffer.End();

        device.SubmitCommandBuffers({
            .commandBuffers = { &commandBuffer },
            .waitSemaphores = { &renderSemaphores[frameIndex] },
            .signalSemaphores = { &presentSemaphores[imageIndex] },
            .waitStageMask = Nova::PipelineStageFlag::ColorAttachmentOutput,
            .fence = &fences[frameIndex]
        });

        frameIndex = (frameIndex + 1) % maxFramesInFlight;

        device.Present(swapchain, imageIndex, { &presentSemaphores[imageIndex] });

        window.PollEvents();
        float elapsedTime = window.GetTimeSeconds() - lastTime;
        lastTime = window.GetTimeSeconds();

        if (window.ShouldClose()) {
            quit = true;
        }

        if (updateSwapchain) {
            updateSwapchain = false;
            device.WaitIdle();
            swapchain.Destroy();
            swapchain = device.CreateSwapchainBuilder(window.GetSurface())
                .SetDesiredFormat(Nova::ImageFormat::B8G8R8A8Unorm)
                .SetDesiredColorSpace(Nova::ColorSpace::SrgbNonlinear)
                .SetDesiredPresentMode(Nova::SwapchainPresentMode::Fifo)
                .SetDesiredExtent(window.GetSize())
                .build();
            depthImage.Destroy();
            depthImage = device.CreateImage({
                .format = device.GetSupportedDepthFormats()[0],
                .extent = { window.GetWidth(), window.GetHeight(), 1 },
                .usage = Nova::ImageUsageFlag::DepthStencilAttachment
            });
            depthImageView.Destroy();
            depthImageView = depthImage.CreateView(Nova::ImageAspectFlag::Depth);
        }
    }
    device.WaitIdle();
    
    return 0;
}
