#pragma once

#include <volk.h>

namespace Nova {
    class Device;

    struct Fence {
        void Destroy();
        ~Fence() { Destroy(); }
    private:
        VkFence m_fence;
        const Device* m_device;

        friend class Device;
    };

    struct Semaphore {
        void Destroy();
        ~Semaphore() { Destroy(); }
    private:
        VkSemaphore m_semaphore;
        const Device* m_device;

        friend class Device;
    };
}