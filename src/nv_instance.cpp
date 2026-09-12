#include "nv_instance.h"
#include "nv_helpers.h"

#include <vector>
#include <GLFW/glfw3.h>

namespace {
    static bool sVolkLoaded = false;
    static bool sGLFWInitialized = false;
    static bool sValidationLayersEnabled = true;

    const std::vector<const char*> sValidationLayers = {
        "VK_LAYER_KHRONOS_validation"
    };

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
            Nova::LogError("{}", msg);
        } else if (messageSeverity & VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT) {
            Nova::LogWarn("{}", msg);
        } else if (messageSeverity & VK_DEBUG_UTILS_MESSAGE_SEVERITY_INFO_BIT_EXT) {
            Nova::LogInfo("{}", msg);
        } else {
            Nova::LogDebug("{}", msg);
        }

        return VK_FALSE;
    }

    void sGetInstanceExtensions(std::vector<const char*>& extensions)
    {
        if (!sGLFWInitialized) {
            if (!glfwInit()) {
                Nova::LogError("Failed to initialize GLFW");
                exit(-1);
            }
            if (!glfwVulkanSupported()) {
                Nova::LogError("GLFW: Vulkan not supported");
                exit(-1);
            }
            sGLFWInitialized = true;
        }
        uint32_t glfwExtensionCount = 0;
        const char** glfwExtensions = glfwGetRequiredInstanceExtensions(&glfwExtensionCount);
        for (uint32_t i = 0; i < glfwExtensionCount; i++) {
            extensions.push_back(glfwExtensions[i]);
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

    VkDebugUtilsMessengerEXT sSetupDebugMessenger(Nova::Instance& instance, PFN_vkDebugUtilsMessengerCallbackEXT pfnUserCallback)
    {
        if (!sValidationLayersEnabled) return VK_NULL_HANDLE;

        VkDebugUtilsMessengerCreateInfoEXT createInfo{};
        populateDebugMessengerCreateInfo(createInfo, pfnUserCallback);

        VkDebugUtilsMessengerEXT debugMessenger{};
        CheckVk(vkCreateDebugUtilsMessengerEXT(instance.GetRaw(), &createInfo, nullptr, &debugMessenger));
        return debugMessenger;
    }

    void sDestroyDebugMessenger(Nova::Instance& instance, VkDebugUtilsMessengerEXT debugMessenger)
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
        if (!sVolkLoaded) {
            volkInitialize();
            sVolkLoaded = true;
        }

        if (sValidationLayersEnabled && !sCheckValidationLayerSupport()) {
            Nova::LogError("Validation layers requested, but not available! Continueing without validation layers.");
            sValidationLayersEnabled = false;
        }

        VkApplicationInfo appInfo{
            .sType = VK_STRUCTURE_TYPE_APPLICATION_INFO,
            .pApplicationName = applicationName.c_str(),
            .applicationVersion = VK_MAKE_VERSION(1, 0, 0),
            .pEngineName = "Nova Engine",
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

namespace Nova {

    Instance::Instance(std::string applicationName)
    {
        Init(applicationName);
    }

    Instance::~Instance()
    {
        if (sValidationLayersEnabled) {
            sDestroyDebugMessenger(*this, m_debugMessenger);
        }
        vkDestroyInstance(m_instance, nullptr);
    }

    void Instance::Init(std::string applicationName)
    {
        sCreateInstance(&m_instance, applicationName);
        volkLoadInstance(m_instance);
        m_debugMessenger = sSetupDebugMessenger(*this, sDefaultDebugCallback);
    }
}