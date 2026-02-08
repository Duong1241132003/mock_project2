// Project includes
#include "models/PlaylistModel.h"

// System includes
#include <algorithm>
#include <random>
#include <sstream>

namespace media_player 
{
namespace models 
{

PlaylistModel::PlaylistModel(const std::string& name)
    : m_id(generateId())
    , m_name(name)
    , m_createdAt(std::chrono::system_clock::now())
    , m_modifiedAt(std::chrono::system_clock::now())
{
}

int PlaylistModel::getTotalDuration() const 
{
    // TODO: Sum durations when metadata is available
    return 0;
}

void PlaylistModel::addItem(const MediaFileModel& media) 
{
    m_items.push_back(media);
    m_modifiedAt = std::chrono::system_clock::now();
}

bool PlaylistModel::removeItem(size_t index) 
{
    if (index >= m_items.size()) 
    {
        return false;
    }
    
    m_items.erase(m_items.begin() + index);
    m_modifiedAt = std::chrono::system_clock::now();
    return true;
}

bool PlaylistModel::removeItem(const std::string& filePath) 
{
    auto it = std::find_if(m_items.begin(), m_items.end(),
        [&filePath](const MediaFileModel& item) {
            return item.getFilePath() == filePath;
        });
    
    if (it != m_items.end()) 
    {
        m_items.erase(it);
        m_modifiedAt = std::chrono::system_clock::now();
        return true;
    }
    
    return false;
}

void PlaylistModel::clear() 
{
    m_items.clear();
    m_modifiedAt = std::chrono::system_clock::now();
}

std::optional<MediaFileModel> PlaylistModel::getItemAt(size_t index) const 
{
    if (index >= m_items.size()) 
    {
        return std::nullopt;
    }
    return m_items[index];
}

bool PlaylistModel::moveItem(size_t fromIndex, size_t toIndex) 
{
    if (fromIndex >= m_items.size() || toIndex >= m_items.size()) 
    {
        return false;
    }
    
    MediaFileModel item = m_items[fromIndex];
    m_items.erase(m_items.begin() + fromIndex);
    m_items.insert(m_items.begin() + toIndex, item);
    m_modifiedAt = std::chrono::system_clock::now();
    
    return true;
}

bool PlaylistModel::containsFile(const std::string& filePath) const 
{
    return findItemIndex(filePath) >= 0;
}

int PlaylistModel::findItemIndex(const std::string& filePath) const 
{
    for (size_t i = 0; i < m_items.size(); i++) 
    {
        if (m_items[i].getFilePath() == filePath) 
        {
            return static_cast<int>(i);
        }
    }
    return -1;
}

std::string PlaylistModel::serialize() const 
{
    std::stringstream ss;
    ss << m_id << "|" << m_name << "|" << m_items.size();
    
    for (const auto& item : m_items) 
    {
        ss << "|" << item.getFilePath();
    }
    
    return ss.str();
}

PlaylistModel PlaylistModel::deserialize(const std::string& data) 
{
    std::stringstream ss(data);
    std::string token;
    
    // Get ID
    std::getline(ss, token, '|');
    std::string id = token;
    
    // Get name
    std::getline(ss, token, '|');
    std::string name = token;
    
    PlaylistModel playlist(name);
    
    // Get count
    std::getline(ss, token, '|');
    
    // Get items
    while (std::getline(ss, token, '|')) 
    {
        if (!token.empty()) 
        {
            MediaFileModel media(token);
            if (media.isValid()) 
            {
                playlist.addItem(media);
            }
        }
    }
    
    return playlist;
}

std::string PlaylistModel::generateId() const 
{
    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_int_distribution<> dis(0, 15);
    
    const char* hex = "0123456789abcdef";
    std::string id;
    id.reserve(32);
    
    for (int i = 0; i < 32; i++) 
    {
        id += hex[dis(gen)];
    }
    
    return id;
}

} // namespace models
} // namespace media_player
