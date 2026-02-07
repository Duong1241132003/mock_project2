// Project includes
#include "utils/Logger.h"

// System includes
#include <iostream>
#include <iomanip>
#include <ctime>

namespace media_player 
{
namespace utils 
{

Logger& Logger::getInstance() 
{
    static Logger instance;
    return instance;
}

Logger::Logger() 
    : m_minLogLevel(LogLevel::DEBUG)
{
    // Try to open default log file
    setLogFile("logs/mediaplayer.log");
}

Logger::~Logger() 
{
    if (m_logFile.is_open()) 
    {
        m_logFile.close();
    }
}

void Logger::log(LogLevel level, const std::string& message) 
{
    if (level < m_minLogLevel) 
    {
        return;
    }
    
    std::lock_guard<std::mutex> lock(m_mutex);
    
    std::string timestamp = getCurrentTimestamp();
    std::string levelStr = logLevelToString(level);
    std::string formattedMessage = "[" + timestamp + "] [" + levelStr + "] " + message;
    
    // Output to console
    std::cout << formattedMessage << std::endl;
    
    // Output to file if open
    if (m_logFile.is_open()) 
    {
        m_logFile << formattedMessage << std::endl;
        m_logFile.flush();
    }
}

void Logger::debug(const std::string& message) 
{
    log(LogLevel::DEBUG, message);
}

void Logger::info(const std::string& message) 
{
    log(LogLevel::INFO, message);
}

void Logger::warning(const std::string& message) 
{
    log(LogLevel::WARNING, message);
}

void Logger::error(const std::string& message) 
{
    log(LogLevel::ERROR, message);
}

void Logger::fatal(const std::string& message) 
{
    log(LogLevel::FATAL, message);
}

void Logger::setLogLevel(LogLevel level) 
{
    m_minLogLevel = level;
}

void Logger::setLogFile(const std::string& filepath) 
{
    std::lock_guard<std::mutex> lock(m_mutex);
    
    if (m_logFile.is_open()) 
    {
        m_logFile.close();
    }
    
    m_logFile.open(filepath, std::ios::out | std::ios::app);
    
    if (!m_logFile.is_open()) 
    {
        std::cerr << "Warning: Could not open log file: " << filepath << std::endl;
    }
}

std::string Logger::getCurrentTimestamp() const 
{
    auto now = std::chrono::system_clock::now();
    auto time = std::chrono::system_clock::to_time_t(now);
    
    std::stringstream ss;
    ss << std::put_time(std::localtime(&time), "%Y-%m-%d %H:%M:%S");
    
    return ss.str();
}

std::string Logger::logLevelToString(LogLevel level) const 
{
    switch (level) 
    {
        case LogLevel::DEBUG:   return "DEBUG";
        case LogLevel::INFO:    return "INFO";
        case LogLevel::WARNING: return "WARNING";
        case LogLevel::ERROR:   return "ERROR";
        case LogLevel::FATAL:   return "FATAL";
        default:                return "UNKNOWN";
    }
}

} // namespace utils
} // namespace media_player
