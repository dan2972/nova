#include <iostream>
#include <spdlog/spdlog.h>

#include "nova.h"
#include "nv_window.h"

int main() {
    Nova::Instance instance;
    Nova::Device device(&instance);
    Nova::Window window(instance, 800, 600, "Nova Engine");
    return 0;
}
