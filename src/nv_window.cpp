#include "nv_window.h"
#include "nv_helpers.h"
#include <unordered_map>

namespace {
    uint32_t sDefaultWidth = 1280;
    uint32_t sDefaultHeight = 720;
    std::unordered_map<GLFWwindow*, nova::Window*> sWindows;
}

namespace nova {
    Window::Window(const Instance& instance, uint32_t width, uint32_t height, const std::string& title) :
        m_instance(&instance), m_width(width), m_height(height), m_title(title) {
        Init();
    }

    void Window::Init() {
        glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
        m_window = glfwCreateWindow(m_width, m_height, m_title.c_str(), nullptr, nullptr);
        if (!m_window) {
            nova::LogError("Failed to create GLFW window");
            exit(-1);
        }

        sWindows[m_window] = this;
        
        CheckVk(glfwCreateWindowSurface(m_instance->GetRaw(), m_window, nullptr, &m_surface.m_surface));
    }

    Window::~Window()
    {
        if (m_surface.m_surface != VK_NULL_HANDLE) {
            vkDestroySurfaceKHR(m_instance->GetRaw(), m_surface.m_surface, nullptr);
            m_surface.m_surface = VK_NULL_HANDLE;
        }

        if (m_window != nullptr) {
            sWindows.erase(m_window);
            glfwDestroyWindow(m_window);
            m_window = nullptr;
        }
    }

    void Window::SetFramebufferResizeCallback(FramebufferResizeCallback callback)
    {
        m_framebufferResizeCallback = callback;
        glfwSetFramebufferSizeCallback(m_window, DispatchFramebufferResize);
    }

    Extent2D Window::GetFramebufferSize() const
    {
        int width, height;
        glfwGetFramebufferSize(m_window, &width, &height);
        return Extent2D{static_cast<uint32_t>(width), static_cast<uint32_t>(height)};
    }

    void Window::DispatchFramebufferResize(GLFWwindow* window, int width, int height)
    {
        const auto it = sWindows.find(window);
        if (it == sWindows.end()) {
            return;
        }
        auto* self = it->second;

        self->m_width = static_cast<uint32_t>(width);
        self->m_height = static_cast<uint32_t>(height);
        if (self->m_framebufferResizeCallback) {
            self->m_framebufferResizeCallback(self, width, height);
        }
    }
}
