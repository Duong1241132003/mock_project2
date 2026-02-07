// Project includes
#include "controllers/QueueController.h"
#include "utils/Logger.h"

namespace media_player 
{
namespace controllers 
{

QueueController::QueueController(std::shared_ptr<models::QueueModel> queueModel)
    : m_queueModel(queueModel)
{
    LOG_INFO("QueueController initialized");
}

QueueController::~QueueController() 
{
    LOG_INFO("QueueController destroyed");
}

void QueueController::addToQueue(const models::MediaFileModel& media) 
{
    m_queueModel->addToEnd(media);
    LOG_INFO("Added to queue: " + media.getFileName());
}

void QueueController::addToQueueNext(const models::MediaFileModel& media) 
{
    m_queueModel->addNext(media);
    LOG_INFO("Added to queue next: " + media.getFileName());
}

void QueueController::addPlaylistToQueue(const models::PlaylistModel& playlist) 
{
    auto items = playlist.getItems();
    
    for (const auto& item : items) 
    {
        m_queueModel->addToEnd(item);
    }
    
    LOG_INFO("Playlist added to queue: " + playlist.getName() + " (" + std::to_string(items.size()) + " items)");
}

void QueueController::addMultipleToQueue(const std::vector<models::MediaFileModel>& mediaList) 
{
    for (const auto& media : mediaList) 
    {
        m_queueModel->addToEnd(media);
    }
    
    LOG_INFO("Added " + std::to_string(mediaList.size()) + " items to queue");
}

bool QueueController::removeFromQueue(size_t index) 
{
    if (m_queueModel->removeAt(index)) 
    {
        LOG_INFO("Removed item from queue at index: " + std::to_string(index));
        return true;
    }
    
    LOG_ERROR("Failed to remove item from queue");
    return false;
}

bool QueueController::removeByPath(const std::string& filePath) 
{
    if (m_queueModel->removeByPath(filePath)) 
    {
        LOG_INFO("Removed item from queue: " + filePath);
        return true;
    }
    
    LOG_ERROR("Failed to remove item from queue");
    return false;
}

void QueueController::clearQueue() 
{
    m_queueModel->clear();
    LOG_INFO("Queue cleared");
}

bool QueueController::jumpToIndex(size_t index) 
{
    if (m_queueModel->jumpTo(index)) 
    {
        LOG_INFO("Jumped to queue index: " + std::to_string(index));
        return true;
    }
    
    LOG_ERROR("Failed to jump to queue index");
    return false;
}

bool QueueController::moveToNext() 
{
    if (m_queueModel->moveToNext()) 
    {
        LOG_INFO("Moved to next item in queue");
        return true;
    }
    
    LOG_ERROR("No next item in queue");
    return false;
}

bool QueueController::moveToPrevious() 
{
    if (m_queueModel->moveToPrevious()) 
    {
        LOG_INFO("Moved to previous item in queue");
        return true;
    }
    
    LOG_ERROR("No previous item in queue");
    return false;
}

bool QueueController::moveItem(size_t fromIndex, size_t toIndex) 
{
    if (m_queueModel->moveItem(fromIndex, toIndex)) 
    {
        LOG_INFO("Moved item in queue from " + std::to_string(fromIndex) + " to " + std::to_string(toIndex));
        return true;
    }
    
    LOG_ERROR("Failed to move item in queue");
    return false;
}

void QueueController::toggleShuffle() 
{
    bool currentShuffle = m_queueModel->isShuffleEnabled();
    m_queueModel->setShuffleMode(!currentShuffle);
    LOG_INFO("Shuffle " + std::string(!currentShuffle ? "enabled" : "disabled"));
}

void QueueController::cycleRepeatMode() 
{
    auto mode = m_queueModel->getRepeatMode();
    models::RepeatMode next = (mode == models::RepeatMode::None) ? models::RepeatMode::LoopOne :
                             (mode == models::RepeatMode::LoopOne) ? models::RepeatMode::LoopAll :
                             models::RepeatMode::None;
    m_queueModel->setRepeatMode(next);
    const char* name = (next == models::RepeatMode::None) ? "None" :
                      (next == models::RepeatMode::LoopOne) ? "LoopOne" : "LoopAll";
    LOG_INFO("Repeat mode: " + std::string(name));
}

void QueueController::toggleRepeat() 
{
    cycleRepeatMode();
}

void QueueController::setShuffle(bool enabled) 
{
    m_queueModel->setShuffleMode(enabled);
    LOG_INFO("Shuffle " + std::string(enabled ? "enabled" : "disabled"));
}

void QueueController::setRepeat(models::RepeatMode mode) 
{
    m_queueModel->setRepeatMode(mode);
}

bool QueueController::isShuffleEnabled() const 
{
    return m_queueModel->isShuffleEnabled();
}

models::RepeatMode QueueController::getRepeatMode() const 
{
    return m_queueModel->getRepeatMode();
}

bool QueueController::isLoopOneEnabled() const 
{
    return m_queueModel->isLoopOneEnabled();
}

bool QueueController::isLoopAllEnabled() const 
{
    return m_queueModel->isLoopAllEnabled();
}

bool QueueController::isRepeatEnabled() const 
{
    return m_queueModel->getRepeatMode() != models::RepeatMode::None;
}

size_t QueueController::getQueueSize() const 
{
    return m_queueModel->size();
}

bool QueueController::isEmpty() const 
{
    return m_queueModel->isEmpty();
}

bool QueueController::hasNext() const 
{
    return m_queueModel->hasNext();
}

bool QueueController::hasPrevious() const 
{
    return m_queueModel->hasPrevious();
}

std::vector<models::MediaFileModel> QueueController::getAllItems() const 
{
    return m_queueModel->getAllItems();
}

std::vector<models::MediaFileModel> QueueController::getPlaybackOrderItems() const
{
    return m_queueModel->getItemsInPlaybackOrder();
}

std::optional<models::MediaFileModel> QueueController::getCurrentItem() const 
{
    return m_queueModel->getCurrentItem();
}

size_t QueueController::getCurrentIndex() const
{
    return m_queueModel->getCurrentIndex();
}

} // namespace controllers
} // namespace media_player