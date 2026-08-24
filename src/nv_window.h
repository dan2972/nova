#pragma once

#include <volk.h>
#include <GLFW/glfw3.h>
#include <string>
#include "nv_instance.h"

namespace Nova {
    class Window {
    public:
        Window();
        Window(const Instance& instance, uint32_t width, uint32_t height, const std::string& title);
    private:
        GLFWwindow* m_window = nullptr;
        uint32_t m_width = 0;
        uint32_t m_height = 0;
        std::string m_title;
        VkSurfaceKHR m_surface = VK_NULL_HANDLE;
        const Instance* m_instance = nullptr;

        void Init();
    };
}