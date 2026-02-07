#ifndef APP_CONFIG_H
#define APP_CONFIG_H

// System includes
#include <string>
#include <vector>

namespace media_player 
{
namespace config 
{

class AppConfig 
{
public:
    static AppConfig& getInstance();
    
    // File system
    static constexpr int MAX_ITEMS_PER_PAGE = 25;
    static constexpr int MAX_SCAN_DEPTH = 10;
    
    // Supported formats
    static const std::vector<std::string> AUDIO_EXTENSIONS;
    static const std::vector<std::string> VIDEO_EXTENSIONS;
    
    // Paths
    static const std::string DEFAULT_COVER_PATH;
    static const std::string PLAYLIST_STORAGE_PATH;
    static const std::string LOG_FILE_PATH;
    static const std::string LIBRARY_STORAGE_PATH;
    static const std::string HISTORY_STORAGE_PATH;
    
    // Serial communication
    static constexpr int SERIAL_BAUD_RATE = 115200;
    static const std::string SERIAL_PORT_DEFAULT;
    
    // Default scan path
    static const std::string DEFAULT_SCAN_PATH;
    
    // Playback - Audio
    static constexpr int DEFAULT_VOLUME = 70;
    static constexpr int PLAYBACK_UPDATE_INTERVAL_MS = 100;
    
    // Playback - Video
    static constexpr int VIDEO_FRAME_QUEUE_SIZE = 3;
    static constexpr int AUDIO_FRAME_QUEUE_SIZE = 9;
    static constexpr double AV_SYNC_THRESHOLD = 0.01;
    static constexpr int DEFAULT_VIDEO_WIDTH = 1280;
    static constexpr int DEFAULT_VIDEO_HEIGHT = 720;
    
    AppConfig(const AppConfig&) = delete;
    AppConfig& operator=(const AppConfig&) = delete;
    
private:
    AppConfig() = default;
};

} // namespace config
} // namespace media_player

#endif // APP_CONFIG_H