#include "nv_buffer.h"

#include "nv_logger.h"

namespace nova {
    BufferDeviceAddress Buffer::GetBufferDeviceAddress()
    {
        if (!m_deviceAddress) {
            LogError("Buffer device address is not available. Ensure the buffer was created with VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT and that the device supports this feature.");
        }

        if ((m_usageFlags & BufferUsageFlag::ShaderDeviceAddress) == BufferUsageFlag::None) {
            LogError("Buffer was not created with VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT.");
        }
        
        return m_deviceAddress;
    }
}