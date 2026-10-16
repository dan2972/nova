#pragma once

#include <volk.h>
#include <GLFW/glfw3.h>
#include <string>
#include <glm/glm.hpp>
#include "nv_instance.h"
#include "nv_surface.h"

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

        bool ShouldClose() const { return glfwWindowShouldClose(m_window); }
        void PollEvents() const { glfwPollEvents(); }

        const Surface& GetSurface() const { return m_surface; }

        Extent2D GetSize() const { return Extent2D{m_width, m_height}; }
        uint32_t GetWidth() const { return m_width; }
        uint32_t GetHeight() const { return m_height; }
    private:
        GLFWwindow* m_window = nullptr;
        uint32_t m_width = 0;
        uint32_t m_height = 0;
        std::string m_title;
        Surface m_surface;
        const Instance* m_instance = nullptr;

        void Init();
    };
}
