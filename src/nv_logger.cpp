#include "nv_logger.h"

namespace {
    static std::unique_ptr<Nova::LoggerBase> s_logger;
}

namespace Nova {
    void SetLogger(LoggerBase* logger) {
        s_logger.reset(logger);
    }

    LoggerBase& GetLogger() {
        if (!s_logger) {
            s_logger = std::make_unique<LoggerDefault>("Nova");
        }
        return *s_logger;
    }
}