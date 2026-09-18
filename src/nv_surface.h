#pragma once

#include <volk.h>

namespace Nova {
    class Window;
    
    class Surface {
    public:
        VkSurfaceKHR GetRaw() const { return m_surface; }
    private:
        VkSurfaceKHR m_surface = VK_NULL_HANDLE;

        friend class Window;
    };
}