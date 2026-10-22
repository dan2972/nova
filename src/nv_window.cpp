#include "nv_window.h"
#include "nv_helpers.h"
#include <SDL3/SDL_vulkan.h>

namespace {
    uint32_t sDefaultWidth = 1280;
    uint32_t sDefaultHeight = 720;
}

namespace nova {
    Window::Window(const Instance& instance, uint32_t width, uint32_t height, const std::string& title) :
        m_instance(&instance), m_width(width), m_height(height), m_title(title) {
        Init();
    }

    void Window::Init() {
        m_window = SDL_CreateWindow(
            m_title.c_str(),
            m_width,
            m_height,
            SDL_WINDOW_VULKAN | SDL_WINDOW_RESIZABLE
        );

        if (!m_window) {
            nova::LogError("Failed to create GLFW window");
            exit(-1);
        }
        
        bool res = SDL_Vulkan_CreateSurface(m_window, m_instance->GetRaw(), nullptr, &m_surface.m_surface);
        if (!res) {
            nova::LogError("Failed to create Vulkan surface");
            exit(-1);
        }
    }

    Window::~Window()
    {
        if (m_surface.m_surface != VK_NULL_HANDLE) {
            vkDestroySurfaceKHR(m_instance->GetRaw(), m_surface.m_surface, nullptr);
            m_surface.m_surface = VK_NULL_HANDLE;
        }

        if (m_window != nullptr) {
            SDL_DestroyWindow(m_window);
            m_window = nullptr;
        }
    }

    void Window::PollEvents()
    {
        m_mouseWheelDeltaX = 0.0f;
        m_mouseWheelDeltaY = 0.0f;
        m_mouseDeltaX = 0.0f;
        m_mouseDeltaY = 0.0f;
        for (SDL_Event event; SDL_PollEvent(&event);) {
            switch (event.type) {
                case SDL_EVENT_QUIT:
                    m_shouldClose = true;
                    break;
                case SDL_EVENT_WINDOW_RESIZED:
                    if (event.window.windowID == SDL_GetWindowID(m_window)) {
                        if (m_framebufferResizeCallback) {
                            m_framebufferResizeCallback(this, event.window.data1, event.window.data2);
                        }
                    }
                    break;
                case SDL_EVENT_MOUSE_MOTION:
                    m_mouseDeltaX = event.motion.xrel;
                    m_mouseDeltaY = event.motion.yrel;
                    if (m_mouseMoveCallback) {
                        m_mouseMoveCallback(this, event.motion.xrel, event.motion.yrel);
                    }
                    break;
                case SDL_EVENT_MOUSE_BUTTON_DOWN:
                case SDL_EVENT_MOUSE_BUTTON_UP:
                    if (m_mouseButtonCallback) {
                        m_mouseButtonCallback(this, static_cast<MouseButton>(event.button.button), event.button.down ? ButtonAction::Press : ButtonAction::Release);
                    }
                    break;
                case SDL_EVENT_MOUSE_WHEEL:
                    if (m_mouseWheelCallback) {
                        m_mouseWheelCallback(this, event.wheel.x, event.wheel.y);
                    }
                    m_mouseWheelDeltaX = event.wheel.x;
                    m_mouseWheelDeltaY = event.wheel.y;
                    break;
                case SDL_EVENT_KEY_DOWN:
                case SDL_EVENT_KEY_UP:
                    if (m_keyCallback) {
                        m_keyCallback(this, static_cast<KeyCode>(event.key.key), static_cast<KeyModifierFlag>(event.key.mod), event.key.down ? KeyAction::Press : KeyAction::Release);
                    }
                    break;
                default:
                    break;
            }
        }
        m_keyStates = SDL_GetKeyboardState(nullptr);
        m_lastKeyModState = SDL_GetModState();
        m_mouseButtonState = SDL_GetMouseState(&m_lastMouseX, &m_lastMouseY);
    }

    void Window::SetFramebufferResizeCallback(FramebufferResizeCallback callback)
    {
        m_framebufferResizeCallback = callback;
    }

    bool Window::GetKeyDown(KeyCode key, KeyModifierFlag mods) const
    {
        if (!m_keyStates) {
            return false;
        }
        SDL_Keycode sdlKey = static_cast<SDL_Keycode>(key);
        if (mods == KeyModifierFlag::None) {
            return m_keyStates[SDL_GetScancodeFromKey(sdlKey, nullptr)];
        }
        SDL_Keymod sdlMods = static_cast<SDL_Keymod>(mods);
        return m_keyStates[SDL_GetScancodeFromKey(sdlKey, nullptr)] && (m_lastKeyModState & sdlMods) == sdlMods;
    }

    bool Window::GetMouseButtonDown(MouseButton button) const
    {
        Uint32 sdlButtonMask = SDL_BUTTON_MASK(static_cast<Uint8>(button));
        return (m_mouseButtonState & sdlButtonMask) != 0;
    }

    void Window::GetMouseMoved(double& dx, double& dy) const
    {
        dx = m_mouseDeltaX;
        dy = m_mouseDeltaY;
    }

    void Window::GetMouseWheelScrolled(double &dx, double &dy) const
    {
        dx = m_mouseWheelDeltaX;
        dy = m_mouseWheelDeltaY;
    }

    Extent2D Window::GetFramebufferSize() const
    {
        int width, height;
        if (!SDL_GetWindowSizeInPixels(m_window, &width, &height)) {
            nova::LogError("Failed to get window size in pixels");
            exit(-1);
        }
        return Extent2D{static_cast<uint32_t>(width), static_cast<uint32_t>(height)};
    }

    Extent2D Window::GetSize() const
    {
        int width, height;
        if (!SDL_GetWindowSize(m_window, &width, &height)) {
            nova::LogError("Failed to get window size");
            exit(-1);
        }
        return Extent2D{static_cast<uint32_t>(width), static_cast<uint32_t>(height)};
    }
}
