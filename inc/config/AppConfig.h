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
    // Supported formats
    static const std::vector<std::string> SUPPORTED_AUDIO_EXTENSIONS;
    static const std::vector<std::string> SUPPORTED_VIDEO_EXTENSIONS;
    
    // Scannable formats (includes supported + unsupported but visible)
    static const std::vector<std::string> SCANNABLE_EXTENSIONS;

    
    // Paths
    static const std::string DEFAULT_COVER_PATH;
    static const std::string PLAYLIST_STORAGE_PATH;
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
    
    AppConfig(const AppConfig&) = delete;
    AppConfig& operator=(const AppConfig&) = delete;
    
private:
    AppConfig() = default;
};

} // namespace config
} // namespace media_player

#endif // APP_CONFIG_H