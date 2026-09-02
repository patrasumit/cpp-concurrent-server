#include "logging/Logger.h"

#include <iostream>
#include <mutex>

Logger& Logger::instance()
{
    static Logger logger;
    return logger;
}

void Logger::log(
    LogLevel level,
    const std::string& message)
{
    std::lock_guard<std::mutex> lock(mutex);

    switch (level)
    {
        case LogLevel::Debug:
            std::cout << "[DEBUG] ";
            break;

        case LogLevel::Info:
            std::cout << "[INFO] ";
            break;

        case LogLevel::Warning:
            std::cout << "[WARNING] ";
            break;

        case LogLevel::Error:
            std::cout << "[ERROR] ";
            break;
    }

    std::cout << message << '\n';
}

void Logger::info(const std::string& message)
{
    log(LogLevel::Info, message);
}

void Logger::debug(const std::string& message)
{
    log(LogLevel::Debug, message);
}

void Logger::warning(const std::string& message)
{
    log(LogLevel::Warning, message);
}

void Logger::error(const std::string& message)
{
    log(LogLevel::Error, message);
}