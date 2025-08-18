#pragma once

#include <string>
#include <sstream>
#include <mutex>

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

    void Log(LogLevel level, const std::string& file, const std::string& func, int line, const std::string& message);

private:
    Logger() = default;
    std::string GetFormattedTime() const;
    std::string GetFileName(const std::string& file) const;
    std::string GetColorCode(LogLevel level) const;

    std::mutex m_log_mutex;
};

#define AIOTEK_LOG(level, format, ...) \
    do { \
        std::stringstream ss; \
        ss << aiotek::core::format_string(format, ##__VA_ARGS__); \
        aiotek::core::Logger::GetInstance().Log(level, __FILE__, __func__, __LINE__, ss.str()); \
    } while (0)

#define AIOTEK_LOG_DEBUG(format, ...) AIOTEK_LOG(aiotek::core::LogLevel::DEBUG, format, ##__VA_ARGS__)
#define AIOTEK_LOG_INFO(format, ...)  AIOTEK_LOG(aiotek::core::LogLevel::INFO, format, ##__VA_ARGS__)
#define AIOTEK_LOG_WARN(format, ...)  AIOTEK_LOG(aiotek::core::LogLevel::WARN, format, ##__VA_ARGS__)
#define AIOTEK_LOG_ERROR(format, ...) AIOTEK_LOG(aiotek::core::LogLevel::ERROR, format, ##__VA_ARGS__)

template<typename... Args>
std::string format_string(const std::string& format, Args... args) {
    std::stringstream ss;
    size_t pos = 0;
    std::tuple<Args...> arg_tuple(args...);
    std::string result = format;

    auto replace_next = [&](const auto& value) {
        size_t found = result.find("{}", pos);
        if (found != std::string::npos) {
            std::stringstream val_ss;
            val_ss << value;
            result.replace(found, 2, val_ss.str());
            pos = found + val_ss.str().length();
        }
    };

    std::apply([&](const auto&... vals) {
        (replace_next(vals), ...);
    }, arg_tuple);

    return result;
}

} // namespace core
} // namespace aiotek