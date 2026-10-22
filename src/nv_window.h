#pragma once

#include <volk.h>
#include <SDL3/SDL.h>
#include <string>
#include "nv_instance.h"
#include "nv_surface.h"
#include "nv_input.h"

namespace nova {
    using FramebufferResizeCallback = void(*)(Window* window, int width, int height);
    using MouseMoveCallback = void(*)(Window* window, double dx, double dy);
    using MouseWheelCallback = void(*)(Window* window, double dx, double dy);
    using MouseButtonCallback = void(*)(Window* window, MouseButton button, ButtonAction action);
    using KeyInputCallback = void(*)(Window* window, KeyCode key, KeyModifierFlag mods, KeyAction action);

    class Window {
    public:
        Window() = default;
        Window(const Instance& instance, uint32_t width, uint32_t height, const std::string& title);
        ~Window();

        Window(const Window&) = delete;
        Window& operator=(const Window&) = delete;
        Window(Window&&) = delete;
        Window& operator=(Window&&) = delete;

        bool ShouldClose() const { return m_shouldClose; }
        void PollEvents();

        void SetFramebufferResizeCallback(FramebufferResizeCallback callback);
        void SetMouseMoveCallback(MouseMoveCallback callback) { m_mouseMoveCallback = callback; }
        void SetMouseWheelCallback(MouseWheelCallback callback) { m_mouseWheelCallback = callback; }
        void SetMouseButtonCallback(MouseButtonCallback callback) { m_mouseButtonCallback = callback; }
        void SetKeyInputCallback(KeyInputCallback callback) { m_keyCallback = callback; }

        bool GetKeyDown(KeyCode key, KeyModifierFlag mods = KeyModifierFlag::None) const;
        bool GetMouseButtonDown(MouseButton button) const;
        void GetMouseMoved(double& dx, double& dy) const;
        void GetMouseWheelScrolled(double& dx, double& dy) const;

        uint64_t GetTimeMilliseconds() const { return SDL_GetTicks(); }
        double GetTimeSeconds() const { return SDL_GetTicks() / 1000.0; }

        const Surface& GetSurface() const { return m_surface; }

        Extent2D GetFramebufferSize() const;
        Extent2D GetSize() const;
        uint32_t GetWidth() const { return GetSize().width; }
        uint32_t GetHeight() const { return GetSize().height; }
        uint32_t GetFramebufferWidth() const { return GetFramebufferSize().width; }
        uint32_t GetFramebufferHeight() const { return GetFramebufferSize().height; }
    private:
        SDL_Window* m_window = nullptr;
        uint32_t m_width = 0;
        uint32_t m_height = 0;
        std::string m_title;
        Surface m_surface;
        const Instance* m_instance = nullptr;
        bool m_shouldClose = false;
        MouseMoveCallback m_mouseMoveCallback = nullptr;
        MouseButtonCallback m_mouseButtonCallback = nullptr;
        MouseWheelCallback m_mouseWheelCallback = nullptr;
        KeyInputCallback m_keyCallback = nullptr;
        FramebufferResizeCallback m_framebufferResizeCallback = nullptr;

        const bool* m_keyStates = nullptr;
        SDL_MouseButtonFlags m_mouseButtonState = 0;
        SDL_Keymod m_lastKeyModState = 0;
        float m_lastMouseX = 0.0f;
        float m_lastMouseY = 0.0f;
        float m_mouseDeltaX = 0.0f;
        float m_mouseDeltaY = 0.0f;
        float m_mouseWheelDeltaX = 0.0f;
        float m_mouseWheelDeltaY = 0.0f;

        void Init();
    };
}
