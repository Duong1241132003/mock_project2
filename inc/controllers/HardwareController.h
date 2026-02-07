#ifndef HARDWARE_CONTROLLER_H
#define HARDWARE_CONTROLLER_H

// System includes
#include <memory>
#include <string>
#include <functional>
#include <mutex>
#include <chrono>

// Project includes
#include "services/SerialCommunication.h"
#include "models/PlaybackStateModel.h"

namespace media_player 
{
namespace controllers 
{

/**
 * @brief Enum định nghĩa các lệnh button từ S32K144
 * BTN 1: Toggle Play/Pause
 * BTN 2: Next track
 * BTN 3: Previous track
 * BTN 4: Quit Application
 */
enum class HardwareButton
{
    TOGGLE_PLAY_PAUSE = 1,
    NEXT = 2,
    PREVIOUS = 3,
    QUIT = 4
};

// Callback types
using HardwareButtonCallback = std::function<void(HardwareButton button)>;
using HardwareVolumeCallback = std::function<void(int volume)>;

/**
 * @brief Controller xử lý giao tiếp UART với bo mạch S32K144
 * 
 * Protocol nhận:
 * - "!ADC:%d!" với d từ 0-100: Điều chỉnh volume
 * - "!BTN:%d!" với d từ 1-4: Thực hiện các chức năng điều khiển
 * - Bản tin khác format sẽ bị loại bỏ
 */
class HardwareController 
{
public:
    HardwareController(
        std::shared_ptr<services::SerialCommunication> serialComm,
        std::shared_ptr<models::PlaybackStateModel> playbackStateModel
    );
    
    ~HardwareController();
    
    // Connection management
    bool initialize();
    bool connect(const std::string& portName, int baudRate);
    void disconnect();
    bool isConnected() const;

    /**
     * @brief Thử kết nối lại phần cứng theo chu kỳ khi chưa kết nối
     */
    void refreshConnection();
    
    /**
     * @brief Tự động quét và kết nối với serial port có sẵn
     * @return true nếu kết nối thành công
     */
    bool autoConnect();
    
    // Send data to hardware
    void sendCurrentSongInfo(const std::string& title, const std::string& artist);
    void sendPlaybackState(bool isPlaying);
    
    // Callbacks from hardware
    void setButtonCallback(HardwareButtonCallback callback);
    void setVolumeCallback(HardwareVolumeCallback callback);
    
private:
    // Serial data parsing
    void onSerialDataReceived(const std::string& data);
    void processBuffer();
    bool parseS32K144Message(const std::string& message);
    
    /**
     * @brief Quét các serial port có sẵn trên hệ thống
     * @return Danh sách các port path (e.g., /dev/ttyUSB0, /dev/ttyACM0)
     */
    std::vector<std::string> scanAvailablePorts();
    
    std::shared_ptr<services::SerialCommunication> m_serialComm;
    std::shared_ptr<models::PlaybackStateModel> m_playbackStateModel;

    // Trạng thái reconnect để thử kết nối lại theo chu kỳ
    std::chrono::steady_clock::time_point m_lastReconnectAttempt;
    std::chrono::milliseconds m_reconnectInterval;
    
    HardwareButtonCallback m_buttonCallback;
    HardwareVolumeCallback m_volumeCallback;
    
    // Buffer để xử lý partial messages từ UART
    std::string m_receiveBuffer;
    mutable std::mutex m_bufferMutex;
};

} // namespace controllers
} // namespace media_player

#endif // HARDWARE_CONTROLLER_H