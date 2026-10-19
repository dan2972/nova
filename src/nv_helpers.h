#pragma once

#include <volk.h>
#include <source_location>
#include "nv_logger.h"

#define ENABLE_BITMASK_OPERATORS(EnumType, UnderlyingType)\
    constexpr EnumType operator|(EnumType a, EnumType b) {\
        return static_cast<EnumType>(static_cast<UnderlyingType>(a) | static_cast<UnderlyingType>(b));\
    }\
    constexpr EnumType operator&(EnumType a, EnumType b) {\
        return static_cast<EnumType>(static_cast<UnderlyingType>(a) & static_cast<UnderlyingType>(b));\
    }\
    constexpr EnumType operator~(EnumType a) {\
        return static_cast<EnumType>(~static_cast<UnderlyingType>(a));\
    }

static inline void CheckVk(VkResult res, const std::source_location& location = std::source_location::current()) {
    if (res == 0) return;
    nova::LogError("Vulkan error: {} at {}:{}", static_cast<int>(res), location.file_name(), location.line());
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

namespace nova {
    struct Extent3D {
        uint32_t width;
        uint32_t height;
        uint32_t depth;

        operator VkExtent3D() const {
            return VkExtent3D{width, height, depth};
        }
    };

    struct Extent2D {
        uint32_t width;
        uint32_t height;
        
        operator VkExtent2D() const {
            return VkExtent2D{width, height};
        }
    };
}