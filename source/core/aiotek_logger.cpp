#include "aiotek_logger.hpp"
#include <iostream>
#include <iomanip>
#include <chrono>
#include <sys/resource.h>

namespace aiotek {
namespace core {

Logger& Logger::GetInstance() {
    static Logger instance;
    return instance;
}

std::string Logger::GetFormattedTime() const {
    auto now = std::chrono::system_clock::now();
    auto time_t = std::chrono::system_clock::to_time_t(now);
    auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(now.time_since_epoch()) % 1000;

    std::stringstream ss;
    ss << std::put_time(std::localtime(&time_t), "%Y-%m-%d %H:%M:%S");
    ss << '.' << std::setfill('0') << std::setw(3) << ms.count();
    return ss.str();
}

std::string Logger::GetFileName(const std::string& file) const {
    size_t pos = file.find_last_of("/\\");
    return pos == std::string::npos ? file : file.substr(pos + 1);
}

std::string Logger::GetColorCode(LogLevel level) const {
    switch (level) {
        case LogLevel::INFO:  return "\033[32m";
        case LogLevel::WARN:  return "\033[33m";
        case LogLevel::ERROR: return "\033[31m";
        case LogLevel::DEBUG: return "\033[0m";
        default: return "\033[0m";
    }
}

void Logger::Log(LogLevel level, const std::string& file, int line, const std::string& message) {
    std::lock_guard<std::mutex> lock(m_log_mutex);

    std::stringstream ss;
    ss << "[" << GetFormattedTime() << "][" << GetFileName(file) << "][" << line << "]: " << message;

    std::cout << GetColorCode(level) << ss.str() << "\033[0m" << std::endl;
}

} // namespace core
} // namespace aiotek
#