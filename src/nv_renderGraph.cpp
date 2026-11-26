#include "nv_renderGraph.h"

#include "nv_device.h"
#include <algorithm>
#include <bit>
#include <deque>
#include <stdexcept>

namespace nova {
    namespace {
        struct State {
            ImageLayout layout = ImageLayout::Undefined;
            SyncScope scope{};
        };

        struct UseGroup {
            size_t firstPass;
            size_t lastPass;
            State state;
        };
        using Timeline = std::vector<UseGroup>;

        struct BufferInfo {
            BufferSize size;
            BufferUsageFlag usage;
            AllocationFlag allocationFlags;
            bool operator==(const BufferInfo&) const = default;
        };

        template<typename Wrapper, typename Info>
        struct Resource {
            const Wrapper* imported = nullptr;
            Info info{};
            State initial{};
        };

        template<typename Wrapper>
        struct Storage {
            std::deque<Wrapper> owned;
            std::vector<const Wrapper*> resolved;
        };

        struct Plan {
            Storage<Image> images;
            Storage<Buffer> buffers;
            std::vector<BarrierBatch> barriers;
        };

        void AppendUse(Timeline& uses, size_t pass, State state, const std::string& name)
        {
            if (!uses.empty()) {
                auto& previous = uses.back();
                if (previous.lastPass == pass) {
                    throw std::invalid_argument("Duplicate resource use in pass " + name);
                }
                if (previous.state.layout == ImageLayout::PresentSrc) {
                    throw std::invalid_argument("Image used after present in pass " + name);
                }
                // One incoming dependency makes writes visible to every compatible reader.
                // The outgoing dependency must wait for every one of those readers too.
                if (previous.state.layout == state.layout && !previous.state.scope.HasWrites() && !state.scope.HasWrites()) {
                    previous.state.scope = previous.state.scope | state.scope;
                    previous.lastPass = pass;
                    return;
                }
            }
            uses.push_back({pass, pass, state});
        }

        Image Allocate(const Device& device, const ImageCreateInfo& info)
        {
            return device.CreateImage(info);
        }

        Buffer Allocate(const Device& device, const BufferInfo& info)
        {
            return device.CreateBuffer(info.size, info.usage, info.allocationFlags);
        }

        bool Matches(const ImageCreateInfo& a, const ImageCreateInfo& b)
        {
            return a.format == b.format && a.extent.width == b.extent.width && a.extent.height == b.extent.height &&
                a.extent.depth == b.extent.depth && a.mipLevels == b.mipLevels && a.usage == b.usage &&
                a.allocationFlags == b.allocationFlags;
        }

        bool Matches(const BufferInfo& a, const BufferInfo& b) { return a == b; }

        void AddBarrier(BarrierBatch& batch, const Image& image, State src, State dst)
        {
            batch.Add(ImageTransition{&image, src.layout, dst.layout, src.scope, dst.scope,
                {.aspect = image.GetAspectMask(), .levelCount = image.GetMipLevels()}});
        }

        void AddBarrier(BarrierBatch& batch, const Buffer& buffer, State src, State dst)
        {
            batch.Add(BufferBarrier{&buffer, src.scope, dst.scope});
        }

        template<typename Wrapper, typename Info>
        void CompileResources(const Device& device, const std::vector<Resource<Wrapper, Info>>& resources,
                              const std::vector<Timeline>& uses, Storage<Wrapper>& storage, std::vector<BarrierBatch>& barriers)
        {
            std::vector<size_t> order;
            storage.resolved.resize(resources.size());
            for (size_t i = 0; i < uses.size(); ++i) {
                storage.resolved[i] = resources[i].imported;
                if (!uses[i].empty()) order.push_back(i);
            }
            std::stable_sort(order.begin(), order.end(), [&](size_t a, size_t b) {
                return uses[a].front().firstPass < uses[b].front().firstPass;
            });

            // Each slot refers to the last logical resource placed in that allocation.
            std::vector<size_t> slots;
            for (size_t i : order) {
                const auto& resource = resources[i];
                State previous = resource.initial;
                const Wrapper* physical = resource.imported;
                if (!physical) {
                    auto slot = std::find_if(slots.begin(), slots.end(), [&](size_t occupant) {
                        return uses[occupant].back().lastPass < uses[i].front().firstPass &&
                            Matches(resources[occupant].info, resource.info);
                    });
                    if (slot == slots.end()) {
                        physical = &storage.owned.emplace_back(Allocate(device, resource.info));
                        // Host accesses can extend beyond the declared GPU pass lifetime.
                        constexpr auto hostFlags = AllocationFlag::Mapped | AllocationFlag::HostAccessRandom |
                                                   AllocationFlag::HostAccessSequentialWrite;
                        if ((resource.info.allocationFlags & hostFlags) == AllocationFlag::None) slots.push_back(i);
                    } else {
                        physical = storage.resolved[*slot];
                        previous = uses[*slot].back().state;
                        *slot = i;
                    }
                    storage.resolved[i] = physical;
                }

                for (const auto& group : uses[i]) {
                    if (previous.scope.HasStage() || previous.layout != group.state.layout) {
                        AddBarrier(barriers[group.firstPass], *physical, previous, group.state);
                    }
                    previous = group.state;
                }
            }
        }

        void ValidateInitialScope(SyncScope scope)
        {
            if (scope.HasAccess() && !scope.HasStage()) {
                throw std::invalid_argument("An initial access scope requires a pipeline stage.");
            }
        }
    }

    struct RenderGraph::Impl {
        std::vector<Resource<Image, ImageCreateInfo>> images;
        std::vector<Resource<Buffer, BufferInfo>> buffers;
        std::vector<RenderGraphPass> passes;
        std::unique_ptr<Plan> plan;
    };

    RenderGraph::RenderGraph() : m_impl(std::make_unique<Impl>()) {}
    RenderGraph::~RenderGraph() = default;

    void RenderGraph::RequireMutable() const
    {
        if (m_impl->plan) throw std::logic_error("Render graph is already compiled.");
    }

    GraphImage RenderGraph::ImportImage(const Image& image, ImageLayout initialLayout, SyncScope initialScope)
    {
        RequireMutable();
        ValidateInitialScope(initialScope);

        if (!image) throw std::invalid_argument("Cannot import an empty image.");

        for (const auto& resource : m_impl->images) {
            if (resource.imported && resource.imported->GetRaw() == image.GetRaw()) {
                throw std::invalid_argument("Image already imported into render graph.");
            }
        }

        m_impl->images.push_back({.imported = &image, .initial = {initialLayout, initialScope}});
        return {this, m_impl->images.size() - 1};
    }

    GraphBuffer RenderGraph::ImportBuffer(const Buffer& buffer, SyncScope initialScope)
    {
        RequireMutable();
        ValidateInitialScope(initialScope);

        if (!buffer.GetRaw()) throw std::invalid_argument("Cannot import an empty buffer.");

        for (const auto& resource : m_impl->buffers) {
            if (resource.imported && resource.imported->GetRaw() == buffer.GetRaw()) {
                throw std::invalid_argument("Buffer already imported into render graph.");
            }
        }

        m_impl->buffers.push_back({.imported = &buffer, .initial = {.scope = initialScope}});
        return {this, m_impl->buffers.size() - 1};
    }

    GraphImage RenderGraph::CreateTransientImage(const ImageCreateInfo& createInfo)
    {
        RequireMutable();

        auto maxMipLevels = std::bit_width(std::max(createInfo.extent.width, createInfo.extent.height));
        if (!createInfo.extent.width || !createInfo.extent.height || createInfo.extent.depth != 1 ||
            !createInfo.mipLevels || createInfo.mipLevels > maxMipLevels || createInfo.usage == ImageUsageFlag::None) {
            throw std::invalid_argument("Transient image requires a valid 2D extent, mip count and usage.");
        }

        m_impl->images.push_back({.info = createInfo});

        return {this, m_impl->images.size() - 1};
    }

    GraphBuffer RenderGraph::CreateTransientBuffer(BufferSize size, BufferUsageFlag usage, AllocationFlag flags)
    {
        RequireMutable();
        if (!size || usage == BufferUsageFlag::None) throw std::invalid_argument("Transient buffer requires size and usage.");

        m_impl->buffers.push_back({.info = {size, usage, flags}});

        return {this, m_impl->buffers.size() - 1};
    }

    void RenderGraph::AddPass(RenderGraphPass pass)
    {
        RequireMutable();
        m_impl->passes.push_back(std::move(pass));
    }

    void RenderGraph::Compile(const Device& device)
    {
        RequireMutable();
        std::vector<Timeline> imageUses(m_impl->images.size());
        std::vector<Timeline> bufferUses(m_impl->buffers.size());

        for (size_t p = 0; p < m_impl->passes.size(); ++p) {
            const auto& pass = m_impl->passes[p];
            for (const auto& use : pass.images) {
                if (use.image.m_graph != this) throw std::invalid_argument("Invalid image handle in pass " + pass.name);

                bool present = use.layout == ImageLayout::PresentSrc;
                if (use.layout == ImageLayout::Undefined || (present ? use.scope.HasAccess() :
                    (!use.scope.HasStage() || !use.scope.HasAccess()))) {
                    throw std::invalid_argument("Invalid image layout or scope in pass " + pass.name);
                }

                size_t i = use.image.m_index;
                if (imageUses[i].empty() && m_impl->images[i].initial.layout == ImageLayout::Undefined && !use.scope.HasWrites()) {
                    throw std::invalid_argument("First use of undefined image must initialize it in pass " + pass.name);
                }

                if (present && !m_impl->images[i].imported) throw std::invalid_argument("Cannot present a transient image.");
                AppendUse(imageUses[i], p, {use.layout, use.scope}, pass.name);
            }

            for (const auto& use : pass.buffers) {
                if (use.buffer.m_graph != this) throw std::invalid_argument("Invalid buffer handle in pass " + pass.name);
                
                if (!use.scope.HasStage() || !use.scope.HasAccess()) {
                    throw std::invalid_argument("Buffer use requires a stage and access in pass " + pass.name);
                }
                
                size_t i = use.buffer.m_index;
                if (bufferUses[i].empty() && !m_impl->buffers[i].imported && !use.scope.HasWrites()) {
                    throw std::invalid_argument("First use of transient buffer must initialize it in pass " + pass.name);
                }
                AppendUse(bufferUses[i], p, {.scope = use.scope}, pass.name);
            }
        }

        auto plan = std::make_unique<Plan>();
        plan->barriers.resize(m_impl->passes.size());
        CompileResources(device, m_impl->images, imageUses, plan->images, plan->barriers);
        CompileResources(device, m_impl->buffers, bufferUses, plan->buffers, plan->barriers);
        m_impl->plan = std::move(plan);
    }

    void RenderGraph::Record(CommandBuffer& commandBuffer) const
    {
        if (!m_impl->plan) throw std::logic_error("Compile the render graph before recording.");
        for (size_t p = 0; p < m_impl->passes.size(); ++p) {
            commandBuffer.PipelineBarrier(m_impl->plan->barriers[p]);
            const auto& pass = m_impl->passes[p];
            if (pass.record) pass.record(commandBuffer, *this);
        }
    }

    const Image& RenderGraph::GetImage(GraphImage image) const
    {
        if (!m_impl->plan) throw std::logic_error("Compile the render graph before resolving resources.");
        if (image.m_graph != this) throw std::invalid_argument("Invalid render graph image handle.");
        const Image* physical = m_impl->plan->images.resolved[image.m_index];
        if (!physical) throw std::logic_error("Unused transient image has no allocation.");
        return *physical;
    }

    const Buffer& RenderGraph::GetBuffer(GraphBuffer buffer) const
    {
        if (!m_impl->plan) throw std::logic_error("Compile the render graph before resolving resources.");
        if (buffer.m_graph != this) throw std::invalid_argument("Invalid render graph buffer handle.");
        const Buffer* physical = m_impl->plan->buffers.resolved[buffer.m_index];
        if (!physical) throw std::logic_error("Unused transient buffer has no allocation.");
        return *physical;
    }
}
