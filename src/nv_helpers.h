#pragma once

#include <volk.h>
#include <source_location>
#include "nv_logger.h"

static inline void CheckVk(VkResult res, const std::source_location& location = std::source_location::current()) {
    if (res == 0) return;
    Nova::LogError("Vulkan error: {} at {}:{}", static_cast<int>(res), location.file_name(), location.line());
    exit(res);
}

enum class Platform {
    Windows,
    MacOS,
    Linux
};

static inline bool PlatformDefined(Platform platform) {
    switch (platform) {
        case Platform::Windows:
            #if defined(_WIN32)
                return true;
            #else
                return false;
            #endif
        case Platform::MacOS:
            #if defined(__APPLE__)
                return true;
            #else
                return false;
            #endif
        case Platform::Linux:
            #if defined(__linux__)
                return true;
            #else
                return false;
            #endif
        default:
            return false;
    }
}
