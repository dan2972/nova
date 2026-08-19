#pragma once

#include <volk.h>
#include <string>

namespace Nova {
    class Instance {
    public:
        Instance(std::string applicationName = "Nova Application");
        ~Instance();

    private:
        void Init(std::string applicationName);

        VkInstance m_instance;

        friend class Device;
    };
}