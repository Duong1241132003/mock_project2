#ifndef QUEUE_MODEL_H
#define QUEUE_MODEL_H

// System includes
#include <vector>
#include <optional>
#include <memory>

// Project includes
#include "MediaFileModel.h"

namespace media_player 
{
namespace models 
{

enum class RepeatMode { None, LoopOne, LoopAll };

class QueueModel 
{
public:
    QueueModel() = default;
    
    // Queue operations
    void addToEnd(const MediaFileModel& media);
    void addNext(const MediaFileModel& media);
    void addAt(const MediaFileModel& media, size_t position);
    
    bool removeAt(size_t index);
    bool removeByPath(const std::string& filePath);
    void clear();
    
    // Navigation
    std::optional<MediaFileModel> getCurrentItem() const;
    std::optional<MediaFileModel> getNextItem() const;
    std::optional<MediaFileModel> getPreviousItem() const;
    std::optional<MediaFileModel> getItemAt(size_t index) const;
    
    bool moveToNext();
    bool moveToPrevious();
    bool jumpTo(size_t index);
    
    // Query
    size_t getCurrentIndex() const 
    { 
        return m_currentIndex; 
    }
    
    size_t size() const 
    { 
        return m_items.size(); 
    }
    
    bool isEmpty() const 
    { 
        return m_items.empty(); 
    }
    
    bool hasNext() const;
    bool hasPrevious() const;
    
    std::vector<MediaFileModel> getAllItems() const 
    { 
        return m_items; 
    }
    /** Items in playback order (shuffle order when shuffle on, else same as getAllItems). */
    std::vector<MediaFileModel> getItemsInPlaybackOrder() const;
    
    // Reorder
    bool moveItem(size_t fromIndex, size_t toIndex);
    
    // Modes
    void setShuffleMode(bool enabled);
    bool isShuffleEnabled() const 
    { 
        return m_shuffleEnabled; 
    }
    
    void setRepeatMode(RepeatMode mode);
    RepeatMode getRepeatMode() const 
    { 
        return m_repeatMode; 
    }
    bool isLoopOneEnabled() const 
    { 
        return m_repeatMode == RepeatMode::LoopOne; 
    }
    bool isLoopAllEnabled() const 
    { 
        return m_repeatMode == RepeatMode::LoopAll; 
    }
    /** True when LoopOne or LoopAll (for backward compatibility). */
    bool isRepeatEnabled() const 
    { 
        return m_repeatMode != RepeatMode::None; 
    }
    
private:
    void updateShuffleOrder();
    size_t getActualIndex(size_t logicalIndex) const;
    
    std::vector<MediaFileModel> m_items;
    std::vector<size_t> m_shuffleOrder;
    size_t m_currentIndex = 0;
    bool m_shuffleEnabled = false;
    RepeatMode m_repeatMode = RepeatMode::None;
};

} // namespace models
} // namespace media_player

#endif // QUEUE_MODEL_H