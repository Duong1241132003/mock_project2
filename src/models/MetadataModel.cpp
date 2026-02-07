// Project includes
#include "models/MetadataModel.h"

// System includes
#include <sstream>
#include <iomanip>

namespace media_player 
{
namespace models 
{

MetadataModel::MetadataModel(const std::string& filePath) 
{
    (void)filePath; // Will be used when reading metadata
}

void MetadataModel::setCustomTag(const std::string& key, const std::string& value) 
{
    m_customTags[key] = value;
}

std::optional<std::string> MetadataModel::getCustomTag(const std::string& key) const 
{
    auto it = m_customTags.find(key);
    if (it != m_customTags.end()) 
    {
        return it->second;
    }
    return std::nullopt;
}

bool MetadataModel::isComplete() const 
{
    return !m_title.empty() && !m_artist.empty();
}

std::string MetadataModel::getDisplayTitle() const 
{
    if (!m_title.empty()) 
    {
        return m_title;
    }
    return "Unknown Title";
}

std::string MetadataModel::getDisplayArtist() const 
{
    if (!m_artist.empty()) 
    {
        return m_artist;
    }
    return "Unknown Artist";
}

std::string MetadataModel::getFormattedDuration() const 
{
    int hours = m_durationSeconds / 3600;
    int minutes = (m_durationSeconds % 3600) / 60;
    int seconds = m_durationSeconds % 60;
    
    std::stringstream ss;
    
    if (hours > 0) 
    {
        ss << hours << ":";
        ss << std::setfill('0') << std::setw(2) << minutes << ":";
    }
    else 
    {
        ss << minutes << ":";
    }
    
    ss << std::setfill('0') << std::setw(2) << seconds;
    
    return ss.str();
}

} // namespace models
} // namespace media_player
