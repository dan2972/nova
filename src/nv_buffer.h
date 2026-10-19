#pragma once

#include <volk.h>
#include "nv_allocator.h"
#include "nv_helpers.h"

namespace nova {
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

    ENABLE_BITMASK_OPERATORS(BufferUsageFlag, VkBufferUsageFlags)

    class Buffer {
    public:
        Buffer() = default;
        ~Buffer() { Destroy(); }

        Buffer(const Buffer&) = delete;
        Buffer& operator=(const Buffer&) = delete;

        Buffer(Buffer&& other) noexcept { *this = std::move(other); }
        Buffer& operator=(Buffer&& other) noexcept {
            if (this != &other) {
                Destroy();
                m_allocation = std::move(other.m_allocation);
                m_device = other.m_device;
                m_deviceAddress = other.m_deviceAddress;
                m_size = other.m_size;
                m_usageFlags = other.m_usageFlags;
                other.m_device = nullptr;
                other.m_deviceAddress = 0;
                other.m_size = 0;
                other.m_usageFlags = BufferUsageFlag::None;
            }
            return *this;
        }

        void Destroy() {
            m_allocation.Destroy();
            m_size = 0;
        }

        void* GetMappedData() const { return m_allocation.allocationInfo.pMappedData; }
        VkBuffer GetRaw() const { return m_allocation.buffer; }

        BufferSize GetSize() const { return m_size; }
        BufferUsageFlag GetUsageFlags() const { return m_usageFlags; }
        BufferDeviceAddress GetBufferDeviceAddress();

    private:
        BufferAllocation m_allocation;
        const Device* m_device;
        BufferDeviceAddress m_deviceAddress = 0;
        BufferSize m_size = 0;
        BufferUsageFlag m_usageFlags = BufferUsageFlag::None;

        friend class Device;
    };
}
