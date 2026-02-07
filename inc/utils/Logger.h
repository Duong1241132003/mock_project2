#ifndef LOGGER_H
#define LOGGER_H

// System includes
#include <string>
#include <fstream>
#include <mutex>
#include <memory>

namespace media_player 
{
namespace utils 
{

enum class LogLevel 
{
    DEBUG,
    INFO,
    WARNING,
    ERROR,
    FATAL
};

class Logger 
{
public:
    static Logger& getInstance();
    
    void log(LogLevel level, const std::string& message);
    void debug(const std::string& message);
    void info(const std::string& message);
    void warning(const std::string& message);
    void error(const std::string& message);
    void fatal(const std::string& message);
    
    void setLogLevel(LogLevel level);
    void setLogFile(const std::string& filepath);
    
    // Delete copy constructor and assignment
    Logger(const Logger&) = delete;
    Logger& operator=(const Logger&) = delete;
    
private:
    Logger();
    ~Logger();
    
    std::string getCurrentTimestamp() const;
    std::string logLevelToString(LogLevel level) const;
    
    LogLevel m_minLogLevel;
    std::ofstream m_logFile;
    std::mutex m_mutex;
};

// Convenience macros
#define LOG_DEBUG(msg) media_player::utils::Logger::getInstance().debug(msg)
#define LOG_INFO(msg) media_player::utils::Logger::getInstance().info(msg)
#define LOG_WARNING(msg) media_player::utils::Logger::getInstance().warning(msg)
#define LOG_ERROR(msg) media_player::utils::Logger::getInstance().error(msg)
#define LOG_FATAL(msg) media_player::utils::Logger::getInstance().fatal(msg)

} // namespace utils
} // namespace media_player

#endif // LOGGER_H