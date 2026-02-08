// Project includes
#include "services/SerialCommunication.h"

// System includes (Linux serial communication)
#include <fcntl.h>
#include <unistd.h>
#include <termios.h>
#include <cstring>
#include <cerrno>
#include <filesystem>

namespace media_player 
{
namespace services 
{

SerialCommunication::SerialCommunication()
    : m_fileDescriptor(-1)
    , m_isOpen(false)
    , m_shouldStop(false)
{
}

SerialCommunication::~SerialCommunication() 
{
    close();
}

bool SerialCommunication::open(const std::string& portName, int baudRate) 
{
    std::lock_guard<std::mutex> lock(m_mutex);
    
    if (m_isOpen) 
    {
        return true;
    }
    
    

    std::error_code errorCode;
    if (!std::filesystem::exists(portName, errorCode))
    {
        std::string errorMessage = "Serial port not found: " + portName;
        if (errorCode)
        {
            errorMessage += " (" + errorCode.message() + ")";
        }
        notifyError("Serial port not found");
        return false;
    }

    if (errorCode)
    {
    }

    const auto status = std::filesystem::status(portName, errorCode);
    if (!errorCode && !std::filesystem::is_character_file(status))
    {
    }

    if (::access(portName.c_str(), R_OK | W_OK) != 0)
    {
        const int accessError = errno;
        notifyError("Serial port permission denied");
        return false;
    }
    
    // Open serial port
    m_fileDescriptor = ::open(portName.c_str(), O_RDWR | O_NOCTTY | O_NDELAY);
    
    if (m_fileDescriptor == -1) 
    {
        const int openError = errno;
        if (openError == EACCES || openError == EPERM)
        {
        }
        notifyError("Failed to open port: " + std::string(std::strerror(openError)));
        return false;
    }
    
    // Configure serial port
    struct termios options;
    
    if (tcgetattr(m_fileDescriptor, &options) != 0) 
    {
        ::close(m_fileDescriptor);
        m_fileDescriptor = -1;
        return false;
    }
    
    // Set baud rate
    speed_t speed = B115200; // Default to 115200
    
    switch (baudRate) 
    {
        case 9600: speed = B9600; break;
        case 19200: speed = B19200; break;
        case 38400: speed = B38400; break;
        case 57600: speed = B57600; break;
        case 115200: speed = B115200; break;
        default:
            break;
    }
    
    cfsetispeed(&options, speed);
    cfsetospeed(&options, speed);
    
    // 8N1 mode
    options.c_cflag &= ~PARENB; // No parity
    options.c_cflag &= ~CSTOPB; // 1 stop bit
    options.c_cflag &= ~CSIZE;
    options.c_cflag |= CS8; // 8 data bits
    
    // Enable receiver, ignore modem control lines
    options.c_cflag |= (CLOCAL | CREAD);
    
    // Raw input
    options.c_lflag &= ~(ICANON | ECHO | ECHOE | ISIG);
    
    // Raw output
    options.c_oflag &= ~OPOST;
    
    // Apply settings
    if (tcsetattr(m_fileDescriptor, TCSANOW, &options) != 0) 
    {
        ::close(m_fileDescriptor);
        m_fileDescriptor = -1;
        return false;
    }
    
    m_isOpen = true;
    m_shouldStop = false;
    
    // Start read thread
    m_readThread = std::make_unique<std::thread>(&SerialCommunication::readThread, this);
    
    
    return true;
}

void SerialCommunication::close() 
{
    if (!m_isOpen) 
    {
        return;
    }
    
    
    m_shouldStop = true;
    
    if (m_readThread && m_readThread->joinable()) 
    {
        m_readThread->join();
    }
    
    std::lock_guard<std::mutex> lock(m_mutex);
    
    if (m_fileDescriptor != -1) 
    {
        ::close(m_fileDescriptor);
        m_fileDescriptor = -1;
    }
    
    m_isOpen = false;
}

bool SerialCommunication::isOpen() const 
{
    return m_isOpen;
}

bool SerialCommunication::sendData(const std::string& data) 
{
    std::lock_guard<std::mutex> lock(m_mutex);
    
    if (!m_isOpen || m_fileDescriptor == -1) 
    {
        return false;
    }
    
    ssize_t bytesWritten = write(m_fileDescriptor, data.c_str(), data.length());
    
    if (bytesWritten < 0) 
    {
        notifyError("Write failed");
        return false;
    }
    
    
    return true;
}

std::string SerialCommunication::readData() 
{
    std::lock_guard<std::mutex> lock(m_mutex);
    
    if (!m_isOpen || m_fileDescriptor == -1) 
    {
        return "";
    }
    
    char buffer[256];
    ssize_t bytesRead = read(m_fileDescriptor, buffer, sizeof(buffer) - 1);
    
    if (bytesRead > 0) 
    {
        buffer[bytesRead] = '\0';
        return std::string(buffer);
    }
    
    return "";
}

void SerialCommunication::setDataCallback(SerialDataCallback callback) 
{
    m_dataCallback = callback;
}

void SerialCommunication::setErrorCallback(SerialErrorCallback callback) 
{
    m_errorCallback = callback;
}

void SerialCommunication::readThread() 
{
    char buffer[256];
    
    while (!m_shouldStop) 
    {
        int fd = -1;
        ssize_t bytesRead = 0;
        
        // Scope for lock - only hold while accessing shared resources (FD)
        {
            std::lock_guard<std::mutex> lock(m_mutex);
            
            if (!m_isOpen || m_fileDescriptor == -1) 
            {
                // Check if we should exit loop or just wait (if closed but not stopped)
                if (m_shouldStop) break;
                
                // If just closed but thread running, wait a bit
                // Release lock via scope exit
            }
            else
            {
                fd = m_fileDescriptor;
                // Read inside lock because m_fileDescriptor must be valid
                // O_NDELAY is set so this shouldn't block for long
                 bytesRead = read(fd, buffer, sizeof(buffer) - 1);
            }
        } // Lock released here
        
        // Process result outside lock to allow other threads to sendData
        if (fd != -1)
        {
            if (bytesRead > 0) 
            {
                buffer[bytesRead] = '\0';
                std::string data(buffer);
                
                notifyData(data); // Callback outside lock
            }
            else if (bytesRead < 0) 
            {
                if (errno != EAGAIN && errno != EWOULDBLOCK)
                {
                    notifyError("Read error");
                    
                    std::this_thread::sleep_for(std::chrono::milliseconds(100));
                }
            }
        }
        else
        {
            // If FD was invalid but thread running, wait to avoid spinning
             std::this_thread::sleep_for(std::chrono::milliseconds(50));
        }

        // Sleep outside lock to let other threads (sendData) acquire it
        std::this_thread::sleep_for(std::chrono::milliseconds(10));
    }
}

void SerialCommunication::notifyData(const std::string& data) 
{
    if (m_dataCallback) 
    {
        m_dataCallback(data);
    }
}

void SerialCommunication::notifyError(const std::string& error) 
{
    if (m_errorCallback) 
    {
        m_errorCallback(error);
    }
}

} // namespace services
} // namespace media_player
