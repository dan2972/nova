#pragma once

#include "nv_commandBuffer.h"
#include <functional>
#include <memory>
#include <string>

namespace nova {
    class RenderGraph;

    template<typename Resource>
    class RenderGraphHandle {
    public:
        RenderGraphHandle() = default;

    private:
        RenderGraphHandle(const RenderGraph* graph, size_t index) : m_graph(graph), m_index(index) {}
        const RenderGraph* m_graph = nullptr;
        size_t m_index = 0;
        friend class RenderGraph;
    };

    using GraphImage = RenderGraphHandle<Image>;
    using GraphBuffer = RenderGraphHandle<Buffer>;

    struct GraphImageUse {
        GraphImage image;
        ImageLayout layout = ImageLayout::Undefined;
        SyncScope scope{};
    };

    struct GraphBufferUse {
        GraphBuffer buffer;
        SyncScope scope;
    };

    struct RenderGraphPass {
        std::string name;
        std::vector<GraphImageUse> images;
        std::vector<GraphBufferUse> buffers;
        std::function<void(CommandBuffer&, const RenderGraph&)> record;
    };

    // Passes declare whole-resource uses in execution order. Compile once per graph.
    // The device and imported wrappers must outlive the graph and all submitted work.
    class RenderGraph {
    public:
        RenderGraph();
        ~RenderGraph();
        RenderGraph(const RenderGraph&) = delete;
        RenderGraph& operator=(const RenderGraph&) = delete;

        GraphImage ImportImage(const Image& image, ImageLayout initialLayout, SyncScope initialScope = {});
        GraphBuffer ImportBuffer(const Buffer& buffer, SyncScope initialScope = {});
        GraphImage ImportImage(const Image&&, ImageLayout, SyncScope = {}) = delete;
        GraphBuffer ImportBuffer(const Buffer&&, SyncScope = {}) = delete;
        GraphImage CreateTransientImage(const ImageCreateInfo& createInfo);
        GraphBuffer CreateTransientBuffer(BufferSize size, BufferUsageFlag usage, AllocationFlag flags = AllocationFlag::None);
        void AddPass(RenderGraphPass pass);
        void Compile(const Device& device);

        // Call inside a recording command buffer, outside dynamic rendering.
        // Re-record after GPU completion with imports in their declared initial states.
        void Record(CommandBuffer& commandBuffer) const;
        const Image& GetImage(GraphImage image) const;
        const Buffer& GetBuffer(GraphBuffer buffer) const;

    private:
        void RequireMutable() const;
        struct Impl;
        std::unique_ptr<Impl> m_impl;
    };
}
