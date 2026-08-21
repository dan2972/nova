#pragma once

#include <volk.h>
#include <string>

namespace Nova {
    class Instance {
    public:
        Instance(std::string applicationName = "Nova Application");
        ~Instance();

        VkInstance GetRaw() const { return m_instance; }

    private:
        void Init(std::string applicationName);

        VkInstance m_instance;

        friend class Device;
    };
}