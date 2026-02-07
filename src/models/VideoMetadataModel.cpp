// Project includes
#include "models/VideoMetadataModel.h"

// System includes
#include <sstream>
#include <iomanip>

namespace media_player 
{
namespace models 
{

VideoMetadataModel::VideoMetadataModel(const std::string& filePath)
{
    // Constructor will be populated by VideoMetadataReader
}

std::string VideoMetadataModel::getResolutionString() const 
{
    if (m_width == 0 || m_height == 0) 
    {
        return "Unknown";
    }
    
    return std::to_string(m_width) + "x" + std::to_string(m_height);
}

std::string VideoMetadataModel::getFormattedDuration() const 
{
    int hours = m_durationSeconds / 3600;
    int minutes = (m_durationSeconds % 3600) / 60;
    int seconds = m_durationSeconds % 60;
    
    std::ostringstream oss;
    
    if (hours > 0) 
    {
        oss << std::setfill('0') << std::setw(2) << hours << ":";
    }
    
    oss << std::setfill('0') << std::setw(2) << minutes << ":"
        << std::setfill('0') << std::setw(2) << seconds;
    
    return oss.str();
}

std::string VideoMetadataModel::getFormattedBitrate() const 
{
    if (m_bitrate == 0) 
    {
        return "Unknown";
    }
    
    double kbps = m_bitrate / 1000.0;
    
    if (kbps >= 1000) 
    {
        double mbps = kbps / 1000.0;
        std::ostringstream oss;
        oss << std::fixed << std::setprecision(2) << mbps << " Mbps";
        return oss.str();
    }
    
    std::ostringstream oss;
    oss << std::fixed << std::setprecision(0) << kbps << " kbps";
    return oss.str();
}

double VideoMetadataModel::getAspectRatio() const 
{
    if (m_height == 0) 
    {
        return 0.0;
    }
    
    return static_cast<double>(m_width) / static_cast<double>(m_height);
}

bool VideoMetadataModel::isValid() const 
{
    return m_width > 0 && 
           m_height > 0 && 
           m_durationSeconds > 0 && 
           !m_videoCodec.empty();
}

} // namespace models
} // namespace media_player