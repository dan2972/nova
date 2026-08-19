#include <iostream>
#include <spdlog/spdlog.h>

#include "nova.h"

int main() {
    spdlog::set_level(spdlog::level::debug);

    Nova::Instance instance;
    Nova::Device device(&instance);
    return 0;
}
