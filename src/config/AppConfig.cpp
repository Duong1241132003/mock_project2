// Project includes
#include "config/AppConfig.h"

namespace media_player 
{
namespace config 
{

AppConfig& AppConfig::getInstance() 
{
    static AppConfig instance;
    return instance;
}

// Audio formats - Chỉ hỗ trợ các định dạng ổn định để tránh crash
// Audio formats - Formats effectively supported by the playback engine
const std::vector<std::string> AppConfig::SUPPORTED_AUDIO_EXTENSIONS = 
{
    ".mp3",
    ".wav"
};

// Video formats - Formats effectively supported by the video engine
const std::vector<std::string> AppConfig::SUPPORTED_VIDEO_EXTENSIONS = 
{
    ".avi",
    ".mp4"
};

// All formats to scan and show in library (even if not playable)
const std::vector<std::string> AppConfig::SCANNABLE_EXTENSIONS = 
{
    // Supported
    ".mp3", ".wav",
    ".avi", ".mp4",
    
    // Unsupported but partially scannable (metadata only)
    ".flac", ".ogg", ".m4a", ".wma", ".aac",
    ".mkv", ".mov", ".wmv", ".flv", ".webm"
};


// Paths
const std::string AppConfig::DEFAULT_COVER_PATH = "./assets/default_cover.png";
const std::string AppConfig::PLAYLIST_STORAGE_PATH = "./data/playlists";
const std::string AppConfig::LOG_FILE_PATH = "./logs/app.log";
const std::string AppConfig::LIBRARY_STORAGE_PATH = "./data/library";
const std::string AppConfig::HISTORY_STORAGE_PATH = "./data/history";

// Serial
const std::string AppConfig::SERIAL_PORT_DEFAULT = "/dev/ttyUSB0";

// Default scan path
const std::string AppConfig::DEFAULT_SCAN_PATH = "/home/duong/Music";

} // namespace config
} // namespace media_player