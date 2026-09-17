#pragma once

#include <volk.h>
#include <utility>
#include "nv_helpers.h"

namespace Nova {
    class Device;

    enum class CommandPoolCreateFlag : VkCommandPoolCreateFlags {
        None = 0,
        Transient = VK_COMMAND_POOL_CREATE_TRANSIENT_BIT,
        ResetCommandBuffer = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT,
        Protected = VK_COMMAND_POOL_CREATE_PROTECTED_BIT
    };

    ENABLE_BITMASK_OPERATORS(CommandPoolCreateFlag, VkCommandPoolCreateFlags)

    enum class CommandBufferBeginFlag : VkCommandBufferUsageFlags {
        None = 0,
        OneTimeSubmit = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT,
        RenderPassContinue = VK_COMMAND_BUFFER_USAGE_RENDER_PASS_CONTINUE_BIT,
        SimultaneousUse = VK_COMMAND_BUFFER_USAGE_SIMULTANEOUS_USE_BIT
    };

    ENABLE_BITMASK_OPERATORS(CommandBufferBeginFlag, VkCommandBufferUsageFlags)

    class CommandPool {
    public:
        CommandPool() = default;
        ~CommandPool() { Destroy(); }

        CommandPool(const CommandPool&) = delete;
        CommandPool& operator=(const CommandPool&) = delete;

        CommandPool(CommandPool&& other) noexcept { *this = std::move(other); }
        CommandPool& operator=(CommandPool&& other) noexcept {
            if (this != &other) {
                Destroy();
                m_commandPool = other.m_commandPool;
                m_device = other.m_device;
                other.m_commandPool = VK_NULL_HANDLE;
                other.m_device = VK_NULL_HANDLE;
            }
            return *this;
        }

        VkCommandPool GetRaw() const { return m_commandPool; }
        void Destroy() {
            if (m_commandPool != VK_NULL_HANDLE) {
                vkDestroyCommandPool(m_device, m_commandPool, nullptr);
                m_commandPool = VK_NULL_HANDLE;
            }
        }
    private:
        VkCommandPool m_commandPool = VK_NULL_HANDLE;
        VkDevice m_device = VK_NULL_HANDLE;

        friend class Device;
        friend class CommandBuffer;
    };

    class CommandBuffer {
    public:
        CommandBuffer() = default;
        ~CommandBuffer() { Destroy(); }

        CommandBuffer(const CommandBuffer&) = delete;
        CommandBuffer& operator=(const CommandBuffer&) = delete;

        CommandBuffer(CommandBuffer&& other) noexcept { *this = std::move(other); }
        CommandBuffer& operator=(CommandBuffer&& other) noexcept {
            if (this != &other) {
                Destroy();
                m_commandBuffer = other.m_commandBuffer;
                m_device = other.m_device;
                m_commandPool = other.m_commandPool;
                other.m_commandBuffer = VK_NULL_HANDLE;
                other.m_device = VK_NULL_HANDLE;
                other.m_commandPool = VK_NULL_HANDLE;
            }
            return *this;
        }

        VkCommandBuffer GetRaw() const { return m_commandBuffer; }
        void Destroy() {
            if (m_commandBuffer != VK_NULL_HANDLE && m_commandPool != nullptr) {
                vkFreeCommandBuffers(m_device, m_commandPool->GetRaw(), 1, &m_commandBuffer);
                m_commandBuffer = VK_NULL_HANDLE;
            }
        }

        void Begin(CommandBufferBeginFlag flags = CommandBufferBeginFlag::None) {
            VkCommandBufferBeginInfo beginInfo{
                .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO,
                .flags = static_cast<VkCommandBufferUsageFlags>(flags)
            };
            CheckVk(vkBeginCommandBuffer(m_commandBuffer, &beginInfo));
        }

        void End() {
            CheckVk(vkEndCommandBuffer(m_commandBuffer));
        }

    private:
        VkCommandBuffer m_commandBuffer = VK_NULL_HANDLE;
        VkDevice m_device = VK_NULL_HANDLE;
        CommandPool* m_commandPool = nullptr;

        friend class Device;
    };
}