#pragma once

#include <volk.h>
#include "nv_allocator.h"

namespace Nova {
    using BufferDeviceAddress = VkDeviceAddress;
    using BufferSize = VkDeviceSize;

    class Device;

    enum class BufferUsageFlag : VkBufferUsageFlags {
        None = 0,
        TransferSrc = VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
        TransferDst = VK_BUFFER_USAGE_TRANSFER_DST_BIT,
        UniformTexelBuffer = VK_BUFFER_USAGE_UNIFORM_TEXEL_BUFFER_BIT,
        StorageTexelBuffer = VK_BUFFER_USAGE_STORAGE_TEXEL_BUFFER_BIT,
        UniformBuffer = VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
        StorageBuffer = VK_BUFFER_USAGE_STORAGE_BUFFER_BIT,
        IndexBuffer = VK_BUFFER_USAGE_INDEX_BUFFER_BIT,
        VertexBuffer = VK_BUFFER_USAGE_VERTEX_BUFFER_BIT,
        IndirectBuffer = VK_BUFFER_USAGE_INDIRECT_BUFFER_BIT,
        ShaderDeviceAddress = VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT
    };

    constexpr BufferUsageFlag operator|(BufferUsageFlag a, BufferUsageFlag b) {
        return static_cast<BufferUsageFlag>(static_cast<VkBufferUsageFlags>(a) | static_cast<VkBufferUsageFlags>(b));
    }

    constexpr BufferUsageFlag operator&(BufferUsageFlag a, BufferUsageFlag b) {
        return static_cast<BufferUsageFlag>(static_cast<VkBufferUsageFlags>(a) & static_cast<VkBufferUsageFlags>(b));
    }

    constexpr BufferUsageFlag operator~(BufferUsageFlag a) {
        return static_cast<BufferUsageFlag>(~static_cast<VkBufferUsageFlags>(a));
    }

    class Buffer {
    public:
        Buffer() = default;
        ~Buffer() { Destroy(); }

        void Destroy() {
            m_allocation.Destroy(m_device);
            m_size = 0;
        }

        BufferSize GetSize() const { return m_size; }
        BufferUsageFlag GetUsageFlags() const { return m_usageFlags; }
        BufferDeviceAddress GetBufferDeviceAddress();

    private:
        BufferAllocation m_allocation;
        VkDevice m_device = nullptr;
        BufferDeviceAddress m_deviceAddress = 0;
        BufferSize m_size = 0;
        BufferUsageFlag m_usageFlags;

        friend class Device;
    };
}