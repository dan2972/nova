#include <GLFW/glfw3.h>
#include <vector>
#include "nv_instance.h"
#include "nv_helpers.h"

namespace {
    static bool sVolkLoaded = false;

    void sGetInstanceExtensions(std::vector<const char*>& extensions)
    {
        uint32_t glfwExtensionCount = 0;
        const char** glfwExtensions = glfwGetRequiredInstanceExtensions(&glfwExtensionCount);
        for (uint32_t i = 0; i < glfwExtensionCount; i++) {
            extensions.push_back(glfwExtensions[i]);
        }

        if (PlatformDefined(Platform::MacOS)) {
            extensions.push_back(VK_KHR_PORTABILITY_ENUMERATION_EXTENSION_NAME);
            extensions.push_back(VK_KHR_GET_PHYSICAL_DEVICE_PROPERTIES_2_EXTENSION_NAME);
        }
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
        vkDestroyInstance(m_instance, nullptr);
    }

    void Instance::Init(std::string applicationName)
    {
        sCreateInstance(&m_instance, applicationName);
        volkLoadInstance(m_instance);
    }
}