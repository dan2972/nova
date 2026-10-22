#include "nv_instance.h"
#include "nv_helpers.h"

#include <vector>
#include <algorithm>
#include <SDL3/SDL.h>
#include <SDL3/SDL_vulkan.h>

namespace {
    static bool sNovaInitialized = false;
    static bool sValidationLayersEnabled = true;

    const std::vector<const char*> sValidationLayers = {
        "VK_LAYER_KHRONOS_validation"
    };

    bool sInitializeNova()
    {
        if (sNovaInitialized) {
            return true;
        }

        if (!SDL_Init(SDL_INIT_VIDEO)) {
            nova::LogError("Failed to initialize SDL: {}", SDL_GetError());
            return false;
        }
        if (!SDL_Vulkan_LoadLibrary(nullptr)) {
            nova::LogError("Failed to load Vulkan library: {}", SDL_GetError());
            return false;
        }
        if (volkInitialize() != VK_SUCCESS) {
            nova::LogError("Failed to initialize Volk");
            return false;
        }
        sNovaInitialized = true;
        return true;
    }

    void sShutdownNova()
    {
        if (!sNovaInitialized) {
            return;
        }

        nova::DestroyAllocator();
        SDL_QuitSubSystem(SDL_INIT_VIDEO);
        SDL_Quit();
        sNovaInitialized = false;
    }

    bool sCheckValidationLayerSupport() {
        uint32_t layerCount;
        vkEnumerateInstanceLayerProperties(&layerCount, nullptr);

        std::vector<VkLayerProperties> availableLayers(layerCount);
        vkEnumerateInstanceLayerProperties(&layerCount, availableLayers.data());

        for (const char* layerName : sValidationLayers) {
            bool layerFound = false;

            for (const auto& layerProperties : availableLayers) {
                if (strcmp(layerName, layerProperties.layerName) == 0) {
                    layerFound = true;
                    break;
                }
            }

            if (!layerFound) {
                return false;
            }
        }

        return true;
    }

    std::string sGetMessageSeverityString(VkDebugUtilsMessageSeverityFlagBitsEXT severity) {
        switch (severity) {
            case VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT:
                return "VERBOSE";
            case VK_DEBUG_UTILS_MESSAGE_SEVERITY_INFO_BIT_EXT:
                return "INFO";
            case VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT:
                return "WARNING";
            case VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT:
                return "ERROR";
            default:
                return "UNKNOWN";
        }
    }

    std::string sGetMessageTypeString(VkDebugUtilsMessageTypeFlagsEXT type) {
        if (type == 7) return "General | Validation | Performance";
        if (type == 6) return "Validation | Performance";
        if (type == 5) return "General | Performance";
        if (type == 4 /*VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT*/) return "Performance";
        if (type == 3) return "General | Validation";
        if (type == 2 /*VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT*/) return "Validation";
        if (type == 1 /*VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT*/) return "General";
        return "Unknown";
    }

    static VKAPI_ATTR VkBool32 VKAPI_CALL sDefaultDebugCallback(
        VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity,
        VkDebugUtilsMessageTypeFlagsEXT messageType,
        const VkDebugUtilsMessengerCallbackDataEXT* pCallbackData,
        void* pUserData) {
        auto ms = sGetMessageSeverityString(messageSeverity);
        auto mt = sGetMessageTypeString(messageType);
        auto msg = fmt::format("[Vulkan {} {}]: {}", mt, ms, pCallbackData->pMessage);
        if (messageSeverity & VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT) {
            nova::LogError("{}", msg);
        } else if (messageSeverity & VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT) {
            nova::LogWarn("{}", msg);
        } else if (messageSeverity & VK_DEBUG_UTILS_MESSAGE_SEVERITY_INFO_BIT_EXT) {
            nova::LogInfo("{}", msg);
        } else {
            nova::LogDebug("{}", msg);
        }

        return VK_FALSE;
    }

    void sGetInstanceExtensions(std::vector<const char*>& extensions)
    {
        if (!SDL_WasInit(SDL_INIT_VIDEO)) {
            nova::LogError("SDL was not initialized!");
            return;
        }
        uint32_t sdlExtensionCount = 0;
        auto sdlExtensions = SDL_Vulkan_GetInstanceExtensions(&sdlExtensionCount);
        for (uint32_t i = 0; i < sdlExtensionCount; i++) {
            extensions.push_back(sdlExtensions[i]);
        }

        if (PlatformDefined(Platform::MacOS)) {
            extensions.push_back(VK_KHR_PORTABILITY_ENUMERATION_EXTENSION_NAME);
            extensions.push_back(VK_KHR_GET_PHYSICAL_DEVICE_PROPERTIES_2_EXTENSION_NAME);
        }

        if (sValidationLayersEnabled) {
            extensions.push_back(VK_EXT_DEBUG_UTILS_EXTENSION_NAME);
        }
    }

    void populateDebugMessengerCreateInfo(VkDebugUtilsMessengerCreateInfoEXT& createInfo, PFN_vkDebugUtilsMessengerCallbackEXT pfnUserCallback)
    {
        createInfo = {
            .sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT,
            .messageSeverity = VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT |
                               VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT |
                               VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT,
            .messageType = VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT |
                           VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT |
                           VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT,
            .pfnUserCallback = pfnUserCallback,
            .pUserData = nullptr
        };
    }

    VkDebugUtilsMessengerEXT sSetupDebugMessenger(nova::Instance& instance, PFN_vkDebugUtilsMessengerCallbackEXT pfnUserCallback)
    {
        if (!sValidationLayersEnabled) return VK_NULL_HANDLE;

        VkDebugUtilsMessengerCreateInfoEXT createInfo{};
        populateDebugMessengerCreateInfo(createInfo, pfnUserCallback);

        VkDebugUtilsMessengerEXT debugMessenger{};
        CheckVk(vkCreateDebugUtilsMessengerEXT(instance.GetRaw(), &createInfo, nullptr, &debugMessenger));
        return debugMessenger;
    }

    void sDestroyDebugMessenger(nova::Instance& instance, VkDebugUtilsMessengerEXT debugMessenger)
    {
        if (!sValidationLayersEnabled || debugMessenger == VK_NULL_HANDLE) return;

        vkDestroyDebugUtilsMessengerEXT(instance.GetRaw(), debugMessenger, nullptr);
    }

    void sGetInstanceFlags(VkInstanceCreateFlags& flags)
    {
        if (PlatformDefined(Platform::MacOS)) {
            flags |= VK_INSTANCE_CREATE_ENUMERATE_PORTABILITY_BIT_KHR;
        }
    }

    void sCreateInstance(VkInstance* instance, const std::string& applicationName)
    {
        if (sValidationLayersEnabled && !sCheckValidationLayerSupport()) {
            nova::LogError("Validation layers requested, but not available! Continueing without validation layers.");
            sValidationLayersEnabled = false;
        }

        VkApplicationInfo appInfo{
            .sType = VK_STRUCTURE_TYPE_APPLICATION_INFO,
            .pApplicationName = applicationName.c_str(),
            .applicationVersion = VK_MAKE_VERSION(1, 0, 0),
            .pEngineName = "nova Engine",
            .engineVersion = VK_MAKE_VERSION(1, 0, 0),
            .apiVersion = VK_API_VERSION_1_3
        };

        std::vector<const char*> instanceExtensions;
        VkInstanceCreateFlags instanceFlags = 0;
        sGetInstanceExtensions(instanceExtensions);
        sGetInstanceFlags(instanceFlags);

        VkInstanceCreateInfo createInfo{
            .sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO,
            .flags = instanceFlags,
            .pApplicationInfo = &appInfo,
            .enabledExtensionCount = static_cast<uint32_t>(instanceExtensions.size()),
            .ppEnabledExtensionNames = instanceExtensions.data(),
        };

        if (sValidationLayersEnabled) {
            createInfo.enabledLayerCount = static_cast<uint32_t>(sValidationLayers.size());
            createInfo.ppEnabledLayerNames = sValidationLayers.data();

            VkDebugUtilsMessengerCreateInfoEXT debugCreateInfo{};
            populateDebugMessengerCreateInfo(debugCreateInfo, sDefaultDebugCallback);
            createInfo.pNext = (VkDebugUtilsMessengerCreateInfoEXT*) &debugCreateInfo;
        } else {
            createInfo.enabledLayerCount = 0;
            createInfo.pNext = nullptr;
        }

        CheckVk(vkCreateInstance(&createInfo, nullptr, instance));
    }
}

namespace nova {

    Instance::Instance(const std::string& applicationName)
    {
        Init(applicationName);
    }

    Instance::~Instance()
    {
        if (sValidationLayersEnabled) {
            sDestroyDebugMessenger(*this, m_debugMessenger);
        }
        vkDestroyInstance(m_instance, nullptr);
        sShutdownNova();
    }

    void Instance::Init(const std::string& applicationName)
    {
        if (!sNovaInitialized) {
            if (!sInitializeNova()) {
                LogError("Failed to initialize nova.");
                exit(-1);
            }
        }
        sCreateInstance(&m_instance, applicationName);
        volkLoadInstance(m_instance);
        QueryPhysicalDevices();
        SortPhysicalDevicesDefault(m_physicalDevices);
        m_debugMessenger = sSetupDebugMessenger(*this, sDefaultDebugCallback);
    }

    const std::vector<PhysicalDevice>& Instance::QueryPhysicalDevices()
    {
        uint32_t deviceCount = 0;
        CheckVk(vkEnumeratePhysicalDevices(m_instance, &deviceCount, nullptr));
        if (deviceCount == 0) {
            LogError("Failed to find GPUs with Vulkan support.");
            exit(-1);
        }

        std::vector<VkPhysicalDevice> devices(deviceCount);
        CheckVk(vkEnumeratePhysicalDevices(m_instance, &deviceCount, devices.data()));

        m_physicalDevices.clear();
        m_physicalDevices.reserve(deviceCount);
        for (const auto& device : devices) {
            m_physicalDevices.emplace_back(device);
        }
        SortPhysicalDevicesDefault(m_physicalDevices);
        return m_physicalDevices;
    }

    void Instance::SortPhysicalDevicesDefault(std::vector<PhysicalDevice> &physicalDevices) const
    {
        // sort by device type, preferring discrete GPUs,
        // then by amount of VRAM, preferring more VRAM
        std::sort(physicalDevices.begin(), physicalDevices.end(),
            [](nova::PhysicalDevice a, nova::PhysicalDevice b) {
                if (a.GetType() != b.GetType()) {
                    return a.GetType() == nova::PhysicalDevice::GPUType::DiscreteGPU;
                }

                return a.GetVRAMSize() > b.GetVRAMSize();
            });
    }

    const PhysicalDevice& Instance::GetBestPhysicalDevice() const
    {
        return m_physicalDevices[0];
    }
}