#include "nv_shader.h"
#include "nv_device.h"
#include "nv_helpers.h"

#include <array>
#include <fstream>
#include <stdexcept>
#include <utility>

namespace {
    static Slang::ComPtr<slang::IGlobalSession> s_slangGlobalSession;

    void InitGlobalSession() {
        if (!s_slangGlobalSession) {
            slang::createGlobalSession(s_slangGlobalSession.writeRef());
        }
    }
}

namespace nova {
    void ShaderCompiler::WriteSPIRVToDisk(const SPIRVData& spirv, const std::string& outputPath) {
        std::ofstream output(outputPath, std::ios::binary | std::ios::out);
        if (!output) {
            LogError("Failed to open SPIR-V output file: {}", outputPath);
            return;
        }


        if (spirv.Size() > 0) {
            output.write(reinterpret_cast<const char*>(spirv.Data()), static_cast<std::streamsize>(spirv.Size()));
        }
        output.flush();

        if (!output) {
            LogError("Failed to write SPIR-V output file: {}", outputPath);
        }
    }

    ShaderModule ShaderCompiler::SPIRVToShaderModule(const SPIRVData& spirv, const Device* device)
    {
        if (!spirv) {
            throw std::invalid_argument("Cannot create a shader module from empty SPIR-V data");
        }

        VkShaderModuleCreateInfo shaderModuleCreateInfo{
            .sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO,
            .codeSize = spirv.Size(),
            .pCode = spirv.Data()
        };
        VkShaderModule vkShaderModule;
        CheckVk(vkCreateShaderModule(device->GetRaw(), &shaderModuleCreateInfo, nullptr, &vkShaderModule));
        ShaderModule shaderModule;
        shaderModule.m_vkShaderModule = vkShaderModule;
        shaderModule.m_device = device;
        return shaderModule;
    }

    SPIRVData ShaderCompiler::ReadSPIRVFromDisk(const std::string &path)
    {
        std::ifstream input(path, std::ios::binary | std::ios::ate);
        if (!input) {
            LogError("Failed to open SPIR-V input file: {}", path);
            return SPIRVData();
        }

        std::streamsize size = input.tellg();
        if (size <= 0 || size % sizeof(uint32_t) != 0) {
            LogError("Invalid SPIR-V file size: {}", path);
            return SPIRVData();
        }

        input.seekg(0, std::ios::beg);
        std::vector<uint32_t> data(size / sizeof(uint32_t));
        if (!input.read(reinterpret_cast<char*>(data.data()), size)) {
            LogError("Failed to read SPIR-V input file: {}", path);
            return SPIRVData();
        }

        return SPIRVData(std::make_shared<std::vector<uint32_t>>(std::move(data)), static_cast<size_t>(size));
    }

    void ShaderCompiler::BeginSession() {
        if (!s_slangGlobalSession) {
            InitGlobalSession();
        }

        auto slangTargets{ std::to_array<slang::TargetDesc>({ {
            .format = SLANG_SPIRV,
            .profile = s_slangGlobalSession->findProfile("spirv_1_4")
        } })};
        auto slangOptions{ std::to_array<slang::CompilerOptionEntry>({ {
            slang::CompilerOptionName::EmitSpirvDirectly,
            {slang::CompilerOptionValueKind::Int, 1}
        } })};
        slang::SessionDesc slangSessionDesc{
            .targets = slangTargets.data(),
            .targetCount = SlangInt(slangTargets.size()),
            .defaultMatrixLayoutMode = SLANG_MATRIX_LAYOUT_COLUMN_MAJOR,
            .compilerOptionEntries = slangOptions.data(),
            .compilerOptionEntryCount = uint32_t(slangOptions.size())
        };
        s_slangGlobalSession->createSession(slangSessionDesc, m_slangSession.writeRef());
    }

    void ShaderCompiler::EndSession() {
        // ComPtr releases its owned reference when cleared.
        m_slangSession = nullptr;
    }

    SPIRVData ShaderCompiler::CompileToSPIRVFromShaderPath(const std::string& path)
    {
        Slang::ComPtr<slang::IModule> slangModule{
            m_slangSession->loadModuleFromSource("triangle", path.c_str(), nullptr, nullptr)
        };
        if (!slangModule) {
            LogError("Slang failed to load module: {}", path);
            return SPIRVData();
        }

        Slang::ComPtr<ISlangBlob> spirv;
        const SlangResult result = slangModule->getTargetCode(0, spirv.writeRef());
        if (SLANG_FAILED(result) || !spirv) {
            LogError("Slang failed to generate SPIR-V for: {}", path);
            return SPIRVData();
        }

        const size_t byteSize = spirv->getBufferSize();
        const auto* data = static_cast<const uint32_t*>(spirv->getBufferPointer());
        if (byteSize == 0 || byteSize % sizeof(uint32_t) != 0 || data == nullptr) {
            LogError("Slang returned invalid SPIR-V data");
            return SPIRVData();
        }

        std::shared_ptr<ISlangBlob> blobOwner(
            spirv.detach(),
            [](ISlangBlob* blob) { blob->release(); });
        std::shared_ptr<const void> storage(blobOwner, data);
        return SPIRVData(std::move(storage), byteSize);
    }

    ShaderModule ShaderCompiler::CompileToShaderModuleFromShaderPath(const std::string &path, const Device* device)
    {
        SPIRVData spirv = CompileToSPIRVFromShaderPath(path);
        return SPIRVToShaderModule(spirv, device);
    }

    void ShaderModule::Destroy() {
        if (m_vkShaderModule != VK_NULL_HANDLE) {
            vkDestroyShaderModule(m_device->GetRaw(), m_vkShaderModule, nullptr);
            m_vkShaderModule = VK_NULL_HANDLE;
        }
    }
}
