#pragma once

#include <slang.h>
#include <slang-com-ptr.h>
#include <memory>
#include <string>
#include <utility>
#include <volk.h>

namespace nova {
    class Device;
    
    class SPIRVData {
    public:
        SPIRVData() = default;

        const uint32_t* Data() const noexcept {
            return static_cast<const uint32_t*>(m_storage.get());
        }

        size_t Size() const noexcept {
            return m_sizeBytes;
        }

        explicit operator bool() const noexcept {
            return m_storage && m_sizeBytes > 0;
        }

    private:
        SPIRVData(std::shared_ptr<const void> storage, size_t sizeBytes)
            : m_storage(std::move(storage)), m_sizeBytes(sizeBytes) {}

        std::shared_ptr<const void> m_storage;
        size_t m_sizeBytes = 0;

        friend class ShaderCompiler;
    };

    class ShaderModule {
    public:
        ShaderModule() = default;
        ~ShaderModule() { Destroy(); }

        VkShaderModule GetRaw() const { return m_vkShaderModule; }

        void Destroy();

    private:
        VkShaderModule m_vkShaderModule = VK_NULL_HANDLE;
        const Device* m_device = nullptr;

        friend class ShaderCompiler;
    };

    class ShaderCompiler {
    public:
        ShaderCompiler() = default;
        ~ShaderCompiler() = default;

        static void WriteSPIRVToDisk(const SPIRVData& spirv, const std::string& outputPath);
        static ShaderModule SPIRVToShaderModule(const SPIRVData& spirv, const Device* device);
        static SPIRVData ReadSPIRVFromDisk(const std::string& path);
        
        void BeginSession();
        void EndSession();
        SPIRVData CompileToSPIRVFromShaderPath(const std::string& path);
        ShaderModule CompileToShaderModuleFromShaderPath(const std::string& path, const Device* device);

    private:
        Slang::ComPtr<slang::ISession> m_slangSession;
    };
}
