#include "nv_synchronization.h"

#include "nv_device.h"

namespace nova {
    void Fence::Destroy() {
        if (m_fence != VK_NULL_HANDLE) {
            vkDestroyFence(m_device->GetRaw(), m_fence, nullptr);
            m_fence = VK_NULL_HANDLE;
        }
        m_device = nullptr;
    }

    void Semaphore::Destroy() {
        if (m_semaphore != VK_NULL_HANDLE) {
            vkDestroySemaphore(m_device->GetRaw(), m_semaphore, nullptr);
            m_semaphore = VK_NULL_HANDLE;
        }
        m_device = nullptr;
    }
}
