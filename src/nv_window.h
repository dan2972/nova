#pragma once

#include <volk.h>
#include <GLFW/glfw3.h>
#include <string>
#include <glm/glm.hpp>
#include "nv_instance.h"

namespace Nova {
    class Window {
    public:
        Window();
        Window(const Instance& instance, uint32_t width, uint32_t height, const std::string& title);
        ~Window();

        Window(const Window&) = delete;
        Window& operator=(const Window&) = delete;
        Window(Window&&) = delete;
        Window& operator=(Window&&) = delete;

        VkSurfaceKHR GetSurface() const { return m_surface; }
        glm::ivec2 GetSize() const { return glm::ivec2(m_width, m_height); }
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
