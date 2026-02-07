// Project includes
#include "controllers/HardwareController.h"
#include "utils/Logger.h"
#include "config/AppConfig.h"

// System includes
#include <sstream>
#include <regex>
#include <filesystem>
#include <algorithm>
#include <dirent.h>

namespace media_player 
{
namespace controllers 
{

// Constants cho S32K144 protocol
static const int BAUD_RATE_S32K144 = 115200;
static const int ADC_MIN_VALUE = 0;
static const int ADC_MAX_VALUE = 100;
static const int BTN_MIN_VALUE = 1;
static const int BTN_MAX_VALUE = 4;
// Cho phép khoảng trắng sau dấu ':' để phù hợp dữ liệu thực tế từ board
static const std::regex ADC_PATTERN(R"(!ADC:\s*(\d{1,3})\s*!)");
static const std::regex BTN_PATTERN(R"(!BTN:\s*(\d{1,2})\s*!)");

HardwareController::HardwareController(
    std::shared_ptr<services::SerialCommunication> serialComm,
    std::shared_ptr<models::PlaybackStateModel> playbackStateModel
)
    : m_serialComm(serialComm)
    , m_playbackStateModel(playbackStateModel)
    , m_lastReconnectAttempt(std::chrono::steady_clock::now())
    , m_reconnectInterval(std::chrono::milliseconds(2000))
{
    // Setup serial callbacks
    m_serialComm->setDataCallback(
        [this](const std::string& data) 
        {
            onSerialDataReceived(data);
        }
    );
    
    m_serialComm->setErrorCallback(
        [](const std::string& error) 
        {
            LOG_ERROR("Serial error: " + error);
        }
    );
    
    LOG_INFO("HardwareController initialized");
}

HardwareController::~HardwareController() 
{
    disconnect();
    LOG_INFO("HardwareController destroyed");
}

bool HardwareController::initialize() 
{
    // Thử auto connect trước
    if (autoConnect())
    {
        return true;
    }
    
    // Fallback: thử port mặc định từ config
    std::string port = config::AppConfig::SERIAL_PORT_DEFAULT;
    int baudRate = BAUD_RATE_S32K144;
    
    return connect(port, baudRate);
}

bool HardwareController::autoConnect()
{
    LOG_INFO("Auto-connecting to S32K144...");
    
    auto ports = scanAvailablePorts();
    
    for (const auto& port : ports)
    {
        LOG_INFO("Trying port: " + port);
        
        if (connect(port, BAUD_RATE_S32K144))
        {
            LOG_INFO("Successfully connected to S32K144 on port: " + port);
            return true;
        }
    }
    
    LOG_WARNING("Auto-connect failed, no compatible device found");
    return false;
}

std::vector<std::string> HardwareController::scanAvailablePorts()
{
    std::vector<std::string> ports;
    
    // Quét /dev/ttyUSB* và /dev/ttyACM* trên Linux
    const std::vector<std::string> prefixes = {"/dev/ttyUSB", "/dev/ttyACM"};
    
    for (const auto& prefix : prefixes)
    {
        for (int i = 0; i < 10; ++i)
        {
            std::string portPath = prefix + std::to_string(i);
            
            if (std::filesystem::exists(portPath))
            {
                ports.push_back(portPath);
            }
        }
    }

    // Quét các đường dẫn ổn định theo /dev/serial/by-id và /dev/serial/by-path
    const std::vector<std::string> serialDirectories = {"/dev/serial/by-id", "/dev/serial/by-path"};

    for (const auto& directory : serialDirectories)
    {
        if (!std::filesystem::exists(directory))
        {
            continue;
        }

        for (const auto& entry : std::filesystem::directory_iterator(directory))
        {
            if (entry.is_symlink() || entry.is_character_file())
            {
                ports.push_back(entry.path().string());
            }
        }
    }

    // Loại bỏ trùng lặp để tránh thử kết nối nhiều lần
    std::sort(ports.begin(), ports.end());
    ports.erase(std::unique(ports.begin(), ports.end()), ports.end());

    std::ostringstream portListStream;
    if (ports.empty())
    {
        portListStream << "none";
    }
    else
    {
        for (size_t index = 0; index < ports.size(); ++index)
        {
            if (index > 0)
            {
                portListStream << ", ";
            }

            portListStream << ports[index];
        }
    }
    
    LOG_INFO("Found " + std::to_string(ports.size()) + " serial ports: " + portListStream.str());
    
    return ports;
}

bool HardwareController::connect(const std::string& portName, int baudRate) 
{
    LOG_INFO("Connecting to hardware on port: " + portName + " at " + std::to_string(baudRate) + " baud");
    
    if (m_serialComm->open(portName, baudRate)) 
    {
        LOG_INFO("Hardware connected successfully");
        return true;
    }
    
    LOG_DEBUG("Failed to connect to hardware on port: " + portName);
    return false;
}

void HardwareController::disconnect() 
{
    if (m_serialComm->isOpen()) 
    {
        m_serialComm->close();
        LOG_INFO("Hardware disconnected");
    }
    
    // Clear buffer khi disconnect
    std::lock_guard<std::mutex> lock(m_bufferMutex);
    m_receiveBuffer.clear();
}

bool HardwareController::isConnected() const 
{
    return m_serialComm->isOpen();
}

void HardwareController::refreshConnection()
{
    if (isConnected())
    {
        return;
    }

    const auto now = std::chrono::steady_clock::now();
    if (now - m_lastReconnectAttempt < m_reconnectInterval)
    {
        return;
    }

    m_lastReconnectAttempt = now;

    // Thử auto connect trước, nếu không được thì fallback port mặc định
    LOG_INFO("Hardware not connected, attempting reconnect...");
    if (autoConnect())
    {
        return;
    }

    const std::string defaultPort = config::AppConfig::SERIAL_PORT_DEFAULT;
    if (!defaultPort.empty())
    {
        connect(defaultPort, BAUD_RATE_S32K144);
    }
}

void HardwareController::sendCurrentSongInfo(const std::string& title, const std::string& artist) 
{
    if (!m_serialComm->isOpen()) 
    {
        return;
    }
    
    // Protocol gửi đi: SONG|title|artist\n
    std::string message = "SONG|" + title + "|" + artist + "\n";
    
    if (m_serialComm->sendData(message)) 
    {
        LOG_DEBUG("Sent song info to hardware: " + title);
    }
    else 
    {
        LOG_ERROR("Failed to send song info to hardware");
    }
}

void HardwareController::sendPlaybackState(bool isPlaying) 
{
    if (!m_serialComm->isOpen()) 
    {
        return;
    }
    
    // Protocol gửi đi: STATE|PLAYING hoặc STATE|PAUSED\n
    std::string state = isPlaying ? "PLAYING" : "PAUSED";
    std::string message = "STATE|" + state + "\n";
    
    if (m_serialComm->sendData(message)) 
    {
        LOG_DEBUG("Sent playback state to hardware: " + state);
    }
    else 
    {
        LOG_ERROR("Failed to send playback state to hardware");
    }
}

void HardwareController::setButtonCallback(HardwareButtonCallback callback) 
{
    m_buttonCallback = callback;
}

void HardwareController::setVolumeCallback(HardwareVolumeCallback callback) 
{
    m_volumeCallback = callback;
}

void HardwareController::onSerialDataReceived(const std::string& data) 
{
    // Log ở mức INFO để đảm bảo luôn thấy dữ liệu phần cứng nhận vào
    LOG_INFO("Received from hardware: " + data);
    
    // Thêm data vào buffer
    {
        std::lock_guard<std::mutex> lock(m_bufferMutex);
        m_receiveBuffer += data;
    }
    
    // Xử lý buffer để trích xuất các message hoàn chỉnh
    processBuffer();
}

void HardwareController::processBuffer()
{
    std::lock_guard<std::mutex> lock(m_bufferMutex);
    
    // Tìm và xử lý các message hoàn chỉnh theo format !...!
    size_t startPos = 0;
    
    while (startPos < m_receiveBuffer.length())
    {
        // Tìm vị trí bắt đầu message '!'
        size_t msgStart = m_receiveBuffer.find('!', startPos);
        
        if (msgStart == std::string::npos)
        {
            // Không còn message nào, xóa phần đã xử lý
            m_receiveBuffer.erase(0, startPos);
            break;
        }
        
        // Tìm vị trí kết thúc message '!' (không phải '!' đầu tiên)
        size_t msgEnd = m_receiveBuffer.find('!', msgStart + 1);
        
        if (msgEnd == std::string::npos)
        {
            // Message chưa hoàn chỉnh, giữ lại trong buffer
            m_receiveBuffer.erase(0, msgStart);
            break;
        }
        
        // Trích xuất message hoàn chỉnh bao gồm cả 2 dấu '!'
        std::string completeMessage = m_receiveBuffer.substr(msgStart, msgEnd - msgStart + 1);
        
        // Parse message
        parseS32K144Message(completeMessage);
        
        // Di chuyển đến sau message vừa xử lý
        startPos = msgEnd + 1;
    }
    
    // Xóa phần đã xử lý
    if (startPos > 0 && startPos <= m_receiveBuffer.length())
    {
        m_receiveBuffer.erase(0, startPos);
    }
    
    // Giới hạn kích thước buffer để tránh memory leak
    const size_t MAX_BUFFER_SIZE = 1024;
    if (m_receiveBuffer.length() > MAX_BUFFER_SIZE)
    {
        LOG_WARNING("Buffer overflow, clearing...");
        m_receiveBuffer.clear();
    }
}

bool HardwareController::parseS32K144Message(const std::string& message)
{
    std::smatch match;
    
    // Parse ADC message: "!ADC:%d!" - điều chỉnh volume
    if (std::regex_match(message, match, ADC_PATTERN))
    {
        try
        {
            int volume = std::stoi(match[1].str());
            
            // Validate volume từ 0-100, loại bỏ message ngoài range
            if (volume < ADC_MIN_VALUE || volume > ADC_MAX_VALUE)
            {
                LOG_WARNING("ADC value out of range: " + std::to_string(volume));
                return false;
            }
            
            LOG_INFO("S32K144 ADC Volume: " + std::to_string(volume));
            
            if (m_volumeCallback)
            {
                m_volumeCallback(volume);
            }
            
            return true;
        }
        catch (const std::exception& e)
        {
            LOG_ERROR("Failed to parse ADC value: " + message);
        }
    }
    
    // Parse BTN message: "!BTN:%d!" - điều khiển playback
    if (std::regex_match(message, match, BTN_PATTERN))
    {
        try
        {
            int buttonId = std::stoi(match[1].str());
            
            // Validate button ID từ 1-4
            if (buttonId >= BTN_MIN_VALUE && buttonId <= BTN_MAX_VALUE)
            {
                HardwareButton button = static_cast<HardwareButton>(buttonId);
                
                LOG_INFO("S32K144 Button pressed: " + std::to_string(buttonId));
                
                if (m_buttonCallback)
                {
                    m_buttonCallback(button);
                }
                
                return true;
            }
            else
            {
                LOG_WARNING("Invalid button ID: " + std::to_string(buttonId));
            }
        }
        catch (const std::exception& e)
        {
            LOG_ERROR("Failed to parse BTN value: " + message);
        }
    }
    
    LOG_DEBUG("Unknown S32K144 message format: " + message);
    return false;
}

} // namespace controllers
} // namespace media_player