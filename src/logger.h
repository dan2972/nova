#pragma once

#include <string>
#include <spdlog/spdlog.h>
#include <spdlog/sinks/stdout_color_sinks.h>

namespace Nova {
    class LoggerBase {
    public:
        virtual ~LoggerBase() = default;

        enum class Level {
            Info,
            Debug,
            Warn,
            Error,
        };

        virtual void SetLevel(Level level) = 0;
        virtual void Log(Level level, std::string_view message) = 0;
    };

    class Logger : public LoggerBase {
    public:
        explicit Logger(const std::string& name) {
            logger_ = spdlog::stdout_color_mt(name);
        }

        void SetLevel(Level level) override {
            switch (level) {
                case Level::Info:
                    logger_->set_level(spdlog::level::info);
                    break;
                case Level::Debug:
                    logger_->set_level(spdlog::level::debug);
                    break;
                case Level::Warn:
                    logger_->set_level(spdlog::level::warn);
                    break;
                case Level::Error:
                    logger_->set_level(spdlog::level::err);
                    break;
            }
        }
        
        void Log(Level level, std::string_view message) override {
            switch (level) {
                case Level::Info:
                    logger_->info("{}", message);
                    break;
                case Level::Debug:
                    logger_->debug("{}", message);
                    break;
                case Level::Warn:
                    logger_->warn("{}", message);
                    break;
                case Level::Error:
                    logger_->error("{}", message);
                    break;
            }
        }
    private:
        std::shared_ptr<spdlog::logger> logger_;
    };

    void SetLogger(LoggerBase* logger);
    LoggerBase& GetLogger();

    template <typename... Args>
    void LogInfo(fmt::format_string<Args...> fmt, Args&&... args) {
        GetLogger().Log(LoggerBase::Level::Info,
            fmt::format(fmt, std::forward<Args>(args)...));
    }

    template <typename... Args>
    void LogDebug(fmt::format_string<Args...> fmt, Args&&... args) {
        GetLogger().Log(LoggerBase::Level::Debug,
            fmt::format(fmt, std::forward<Args>(args)...));
    }

    template <typename... Args>
    void LogWarn(fmt::format_string<Args...> fmt, Args&&... args) {
        GetLogger().Log(LoggerBase::Level::Warn,
            fmt::format(fmt, std::forward<Args>(args)...));
    }

    template <typename... Args>
    void LogError(fmt::format_string<Args...> fmt, Args&&... args) {
        GetLogger().Log(LoggerBase::Level::Error,
            fmt::format(fmt, std::forward<Args>(args)...));
    }
}
