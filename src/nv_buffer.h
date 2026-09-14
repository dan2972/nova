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

    struct Buffer {
        BufferSize size = 0;
        BufferUsageFlag usageFlags;

        void Destroy() {
            allocation.Destroy(device);
            size = 0;
        }

        Buffer() = default;
        ~Buffer() { Destroy(); }

        BufferDeviceAddress GetBufferDeviceAddress();

    private:
        BufferAllocation allocation;
        VkDevice device = nullptr;
        BufferDeviceAddress deviceAddress = 0;

        friend class Device;
    };
}