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
const std::vector<std::string> AppConfig::AUDIO_EXTENSIONS = 
{
    ".mp3",
    ".wav"
};

// Video formats - Chỉ hỗ trợ các định dạng ổn định để tránh crash
const std::vector<std::string> AppConfig::VIDEO_EXTENSIONS = 
{
    ".avi",
    ".mp4"
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