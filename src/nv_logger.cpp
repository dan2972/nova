#include "nv_logger.h"

namespace {
    static std::unique_ptr<nova::LoggerBase> s_logger;
}

namespace nova {
    void SetLogger(LoggerBase& logger) {
        s_logger.reset(&logger);
    }

    LoggerBase& GetLogger() {
        if (!s_logger) {
            s_logger = std::make_unique<LoggerDefault>("nova");
        }
        return *s_logger;
    }
}