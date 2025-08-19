#pragma once

#include <string>
#include <mutex>
#include <sstream>

namespace aiotek {
namespace core {

enum class LogLevel {
    DEBUG,
    INFO,
    WARN,
    ERROR
};

class Logger {
public:
    static Logger& GetInstance();

    void Log(LogLevel level, const std::string& file, int line, const std::string& message);

private:
    Logger() = default;
    ~Logger() = default;
    Logger(const Logger&) = delete;
    Logger& operator=(const Logger&) = delete;

    std::mutex m_log_mutex;

    std::string GetFormattedTime() const;

    std::string GetFileName(const std::string& file) const;

    std::string GetColorCode(LogLevel level) const;
};

#define AIOTEK_LOG(level, ...) \
    do { \
        std::stringstream ss; \
        ss << __VA_ARGS__; \
        aiotek::core::Logger::GetInstance().Log(level, __FILE__, __LINE__, ss.str()); \
    } while (0)

#define AIOTEK_LOG_DEBUG(...) AIOTEK_LOG(aiotek::core::LogLevel::DEBUG, __VA_ARGS__)
#define AIOTEK_LOG_INFO(...) AIOTEK_LOG(aiotek::core::LogLevel::INFO, __VA_ARGS__)
#define AIOTEK_LOG_WARN(...) AIOTEK_LOG(aiotek::core::LogLevel::WARN, __VA_ARGS__)
#define AIOTEK_LOG_ERROR(...) AIOTEK_LOG(aiotek::core::LogLevel::ERROR, __VA_ARGS__)

} // namespace core
} // namespace aiotek
