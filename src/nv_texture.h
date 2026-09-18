#pragma once

#include <volk.h>
#include <ktx.h>
#include <ktxvulkan.h>
#include "nv_image.h"
#include "nv_allocator.h"
#include "nv_commandBuffer.h"

namespace Nova {
    class Device;
    class Instance;

    class Texture {
    public:
        Texture() = default;
        ~Texture() { Destroy(); }

        Texture(const Texture&) = delete;
        Texture& operator=(const Texture&) = delete;

        Texture(Texture&& other) noexcept { *this = std::move(other); }
        Texture& operator=(Texture&& other) noexcept {
            if (this != &other) {
                Destroy();
                m_image = std::move(other.m_image);
                m_sampler = other.m_sampler;
                other.m_sampler = VK_NULL_HANDLE;
            }
            return *this;
        }

        static Texture CreateFromFile(const std::string& filePath, const Device& device, const CommandPool& commandPool);

        void Destroy() {
            m_image.Destroy();
            if (m_sampler != VK_NULL_HANDLE) {
                vkDestroySampler(m_image.m_device, m_sampler, nullptr);
                m_sampler = VK_NULL_HANDLE;
            }
        }

        const Image& GetImage() const { return m_image; }

    private:
        Image m_image;
        VkSampler m_sampler = VK_NULL_HANDLE;

        friend class Device;
    };
}