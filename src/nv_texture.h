#pragma once

#include <volk.h>
#include <ktx.h>
#include <ktxvulkan.h>
#include "nv_image.h"
#include "nv_allocator.h"
#include "nv_commandBuffer.h"
#include "nv_sampler.h"

namespace nova {
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
                m_device = other.m_device;
                isArray = other.isArray;
                isCubemap = other.isCubemap;
                isCompressed = other.isCompressed;
                generateMipmaps = other.generateMipmaps;
                baseWidth = other.baseWidth;
                baseHeight = other.baseHeight;
                baseDepth = other.baseDepth;
                numDimensions = other.numDimensions;
                numLevels = other.numLevels;
                numLayers = other.numLayers;
                numFaces = other.numFaces;
                other.m_device = nullptr;
            }
            return *this;
        }

        ImageView CreateView() const;
        ImageView CreateView(ImageAspectFlag aspectMask) const;

        Sampler CreateSampler(const SamplerCreateInfo& createInfo) const;
        Sampler CreateLinearSampler() const;

        void Destroy() {
            m_image.Destroy();
        }

        const Image& GetImage() const { return m_image; }

        bool IsArray() const { return isArray; }
        bool IsCubemap() const { return isCubemap; }
        bool IsCompressed() const { return isCompressed; }
        bool ShouldGenerateMipmaps() const { return generateMipmaps; }
        uint32_t GetBaseWidth() const { return baseWidth; }
        uint32_t GetBaseHeight() const { return baseHeight; }
        uint32_t GetBaseDepth() const { return baseDepth; }
        uint32_t GetNumDimensions() const { return numDimensions; }
        uint32_t GetNumLevels() const { return numLevels; }
        uint32_t GetNumLayers() const { return numLayers; }
        uint32_t GetNumFaces() const { return numFaces; }

    private:
        Image m_image;
        const Device* m_device = nullptr;

        bool isArray;
        bool isCubemap;
        bool isCompressed;
        bool generateMipmaps;
        uint32_t baseWidth;
        uint32_t baseHeight;
        uint32_t baseDepth;
        uint32_t numDimensions;
        uint32_t numLevels;
        uint32_t numLayers;
        uint32_t numFaces;

        friend class Device;
    };
}