#ifndef QUEUE_CONTROLLER_H
#define QUEUE_CONTROLLER_H

// System includes
#include <memory>
#include <vector>

// Project includes
#include "models/QueueModel.h"
#include "models/MediaFileModel.h"
#include "models/PlaylistModel.h"

namespace media_player 
{
namespace controllers 
{

class QueueController 
{
public:
    explicit QueueController(std::shared_ptr<models::QueueModel> queueModel);
    ~QueueController();
    
    // Queue operations
    void addToQueue(const models::MediaFileModel& media);
    void addToQueueNext(const models::MediaFileModel& media);
    void addPlaylistToQueue(const models::PlaylistModel& playlist);
    void addMultipleToQueue(const std::vector<models::MediaFileModel>& mediaList);
    
    bool removeFromQueue(size_t index);
    bool removeByPath(const std::string& filePath);
    void clearQueue();
    
    // Navigation
    bool jumpToIndex(size_t index);
    bool moveToNext();
    bool moveToPrevious();
    
    // Reordering
    bool moveItem(size_t fromIndex, size_t toIndex);
    
    // Modes
    void toggleShuffle();
    void cycleRepeatMode();  // None -> LoopOne -> LoopAll -> None
    void toggleRepeat();    // calls cycleRepeatMode for backward compat
    void setShuffle(bool enabled);
    void setRepeat(models::RepeatMode mode);
    
    // State Query
    bool isShuffleEnabled() const;
    models::RepeatMode getRepeatMode() const;
    bool isLoopOneEnabled() const;
    bool isLoopAllEnabled() const;
    bool isRepeatEnabled() const;  // true when LoopOne or LoopAll
    
    // Queries
    size_t getQueueSize() const;
    bool isEmpty() const;
    bool hasNext() const;
    bool hasPrevious() const;
    
    std::vector<models::MediaFileModel> getAllItems() const;
    /** Items in playback order (reflects shuffle when enabled). */
    std::vector<models::MediaFileModel> getPlaybackOrderItems() const;
    std::optional<models::MediaFileModel> getCurrentItem() const;
    size_t getCurrentIndex() const;
    
private:
    std::shared_ptr<models::QueueModel> m_queueModel;
};

} // namespace controllers
} // namespace media_player

#endif // QUEUE_CONTROLLER_H