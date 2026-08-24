#include "nv_window.h"
#include "nv_helpers.h"

namespace {
    uint32_t sDefaultWidth = 1280;
    uint32_t sDefaultHeight = 720;
}

namespace Nova {
    Window::Window(const Instance& instance, uint32_t width, uint32_t height, const std::string& title) :
        m_instance(&instance), m_width(width), m_height(height), m_title(title) {
        Init();
    }

    void Window::Init() {
        glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
        m_window = glfwCreateWindow(m_width, m_height, m_title.c_str(), nullptr, nullptr);
        if (!m_window) {
            Nova::LogError("Failed to create GLFW window");
            exit(-1);
        }
        
        CheckVk(glfwCreateWindowSurface(m_instance->GetRaw(), m_window, nullptr, &m_surface));
    }
}