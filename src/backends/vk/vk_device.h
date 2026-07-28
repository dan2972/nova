#pragma once

#include <stdexcept>
#include <vulkan/vulkan.h>

#include "../backend_device.h"

namespace Nova::vk {
    class VulkanDevice final : public BackendDevice {
    public:
        VulkanDevice() {
            VkApplicationInfo appInfo{};
            appInfo.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
            appInfo.pApplicationName = "Nova Application";
            appInfo.applicationVersion = VK_MAKE_VERSION(1, 0, 0);
            appInfo.pEngineName = "Nova Engine";
            appInfo.engineVersion = VK_MAKE_VERSION(1, 0, 0);
            appInfo.apiVersion = VK_API_VERSION_1_0;

            VkInstanceCreateInfo createInfo{};
            createInfo.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
            createInfo.pApplicationInfo = &appInfo;

            if (vkCreateInstance(&createInfo, nullptr, &m_instance) != VK_SUCCESS) {
                throw std::runtime_error("Failed to create Vulkan instance");
            }
        }

        ~VulkanDevice() {
            vkDestroyInstance(m_instance, nullptr);
        }
    private:
        VkInstance m_instance;
    };
}