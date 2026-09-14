#pragma once

#include <volk.h>

namespace Nova {
    class Device;

    struct Fence {
        void Destroy() {
            if (fence != VK_NULL_HANDLE) {
                vkDestroyFence(device, fence, nullptr);
                fence = VK_NULL_HANDLE;
            }
        }
        ~Fence() { Destroy(); }
    private:
        VkFence fence;
        VkDevice device;

        friend class Device;
    };

    struct Semaphore {
        void Destroy() {
            if (semaphore != VK_NULL_HANDLE) {
                vkDestroySemaphore(device, semaphore, nullptr);
                semaphore = VK_NULL_HANDLE;
            }
        }
        ~Semaphore() { Destroy(); }
    private:
        VkSemaphore semaphore;
        VkDevice device;

        friend class Device;
    };
}