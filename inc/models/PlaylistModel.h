#ifndef PLAYLIST_MODEL_H
#define PLAYLIST_MODEL_H

// System includes
#include <string>
#include <vector>
#include <memory>
#include <chrono>
#include <optional>

// Project includes
#include "MediaFileModel.h"

namespace media_player 
{
namespace models 
{

class PlaylistModel 
{
public:
    PlaylistModel() = default;
    explicit PlaylistModel(const std::string& name);
    
    // Basic info
    std::string getName() const 
    { 
        return m_name; 
    }
    
    void setName(const std::string& name) 
    { 
        m_name = name; 
    }
    
    std::string getId() const 
    { 
        return m_id; 
    }

    void setId(const std::string& id) 
    {
        m_id = id;
    }
    
    size_t getItemCount() const 
    { 
        return m_items.size(); 
    }
    
    int getTotalDuration() const;
    
    // Item management
    void addItem(const MediaFileModel& media);
    bool removeItem(size_t index);
    bool removeItem(const std::string& filePath);
    void clear();
    
    // Access items
    std::vector<MediaFileModel> getItems() const 
    { 
        return m_items; 
    }
    
    std::optional<MediaFileModel> getItemAt(size_t index) const;
    
    // Reordering
    bool moveItem(size_t fromIndex, size_t toIndex);
    
    // Search
    bool containsFile(const std::string& filePath) const;
    int findItemIndex(const std::string& filePath) const;
    
    // Serialization
    std::string serialize() const;
    static PlaylistModel deserialize(const std::string& data);
    
private:
    std::string generateId() const;
    
    std::string m_id;
    std::string m_name;
    std::vector<MediaFileModel> m_items;
    std::chrono::system_clock::time_point m_createdAt;
    std::chrono::system_clock::time_point m_modifiedAt;
};

} // namespace models
} // namespace media_player

#endif // PLAYLIST_MODEL_H