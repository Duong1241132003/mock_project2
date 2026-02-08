// Project includes
#include "models/MediaFileModel.h"
#include "config/AppConfig.h"

// System includes
#include <filesystem>
#include <algorithm>

namespace fs = std::filesystem;

namespace media_player 
{
namespace models 
{

MediaFileModel::MediaFileModel()
    : m_type(MediaType::UNKNOWN)
    , m_fileSize(0)
{
}

MediaFileModel::MediaFileModel(const std::string& filePath)
    : m_filePath(filePath)
    , m_type(MediaType::UNKNOWN)
    , m_fileSize(0)
{
    extractFileInfo();
    m_type = determineMediaType();
}

bool MediaFileModel::isValid() const 
{
    return !m_filePath.empty() && 
           m_type != MediaType::UNKNOWN && 
           fs::exists(m_filePath);
}

bool MediaFileModel::operator<(const MediaFileModel& other) const 
{
    return m_fileName < other.m_fileName;
}

bool MediaFileModel::operator==(const MediaFileModel& other) const 
{
    return m_filePath == other.m_filePath;
}

std::string MediaFileModel::serialize() const 
{
    return m_filePath + "|" + 
           m_fileName + "|" + 
           m_extension + "|" + 
           std::to_string(static_cast<int>(m_type)) + "|" + 
           std::to_string(m_fileSize);
}

MediaFileModel MediaFileModel::deserialize(const std::string& data) 
{
    // Parse serialized string
    size_t pos = 0;
    size_t nextPos = data.find('|');
    
    if (nextPos == std::string::npos) 
    {
        return MediaFileModel();
    }
    
    std::string filePath = data.substr(pos, nextPos - pos);
    
    return MediaFileModel(filePath);
}

void MediaFileModel::extractFileInfo() 
{
    if (m_filePath.empty()) 
    {
        return;
    }
    
    try 
    {
        fs::path path(m_filePath);
        
        m_fileName = path.filename().string();
        m_extension = path.extension().string();
        
        // Keep extension case-sensitive for specific checks (like .WAV vs .wav)
        // std::transform(m_extension.begin(), m_extension.end(), 
        //               m_extension.begin(), ::tolower);
        
        if (fs::exists(path)) 
        {
            m_fileSize = fs::file_size(path);
            m_lastModified = fs::last_write_time(path);
        }
    }
    catch (const fs::filesystem_error& e) 
    {
    }
}

MediaType MediaFileModel::determineMediaType() const 
{
    // Explicitly block all-uppercase extensions (e.g. .WAV, .MP3) as they might cause crashes
    // But allow .wav (supported)
    // We check if the extension has letters and ALL letters are uppercase
    bool hasLetters = false;
    bool allUpper = true;
    
    for (char c : m_extension) {
        if (std::isalpha(c)) {
            hasLetters = true;
            if (!std::isupper(c)) {
                allUpper = false;
                break;
            }
        }
    }
    
    if (hasLetters && allUpper)
    {
        return MediaType::UNSUPPORTED;
    }

    std::string lowerExt = m_extension;
    std::transform(lowerExt.begin(), lowerExt.end(), lowerExt.begin(), ::tolower);

    // Check against audio extensions
    const auto& audioExts = config::AppConfig::SUPPORTED_AUDIO_EXTENSIONS;
    
    if (std::find(audioExts.begin(), audioExts.end(), lowerExt) != audioExts.end()) 
    {
        return MediaType::AUDIO;
    }
    
    // Check against video extensions
    const auto& videoExts = config::AppConfig::SUPPORTED_VIDEO_EXTENSIONS;
    
    if (std::find(videoExts.begin(), videoExts.end(), lowerExt) != videoExts.end()) 
    {
        return MediaType::VIDEO;
    }
    
    // If we are here, it's likely a scannable but unsupported file (since FileScanner filters by SCANNABLE_EXTENSIONS)
    // But let's double check against SCANNABLE just to be safe, or just assume UNSUPPORTED
    const auto& scannableExts = config::AppConfig::SCANNABLE_EXTENSIONS;
    
    // Use lowerExt for scannable check too
    if (std::find(scannableExts.begin(), scannableExts.end(), lowerExt) != scannableExts.end())
    {
        return MediaType::UNSUPPORTED;
    }
    
    return MediaType::UNKNOWN;
}

} // namespace models
} // namespace media_player
