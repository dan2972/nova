#pragma once

#include <volk.h>

namespace nova {
    class Device;

    struct Fence {
        Fence() = default;
        void Destroy();
        ~Fence() { Destroy(); }

        Fence(const Fence&) = delete;
        Fence& operator=(const Fence&) = delete;

        Fence(Fence&& other) noexcept { *this = static_cast<Fence&&>(other); }
        Fence& operator=(Fence&& other) noexcept {
            if (this != &other) {
                Destroy();
                m_fence = other.m_fence;
                m_device = other.m_device;
                other.m_fence = VK_NULL_HANDLE;
                other.m_device = nullptr;
            }
            return *this;
        }

        VkFence GetRaw() const { return m_fence; }
    private:
        VkFence m_fence = VK_NULL_HANDLE;
        const Device* m_device = nullptr;

        friend class Device;
    };

    struct Semaphore {
        Semaphore() = default;
        void Destroy();
        ~Semaphore() { Destroy(); }

        Semaphore(const Semaphore&) = delete;
        Semaphore& operator=(const Semaphore&) = delete;

        Semaphore(Semaphore&& other) noexcept { *this = static_cast<Semaphore&&>(other); }
        Semaphore& operator=(Semaphore&& other) noexcept {
            if (this != &other) {
                Destroy();
                m_semaphore = other.m_semaphore;
                m_device = other.m_device;
                other.m_semaphore = VK_NULL_HANDLE;
                other.m_device = nullptr;
            }
            return *this;
        }

        VkSemaphore GetRaw() const { return m_semaphore; }
    private:
        VkSemaphore m_semaphore = VK_NULL_HANDLE;
        const Device* m_device = nullptr;

        friend class Device;
    };
}
