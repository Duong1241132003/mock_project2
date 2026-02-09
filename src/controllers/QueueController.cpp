// Project includes
#include "controllers/QueueController.h"

namespace media_player 
{
namespace controllers 
{

QueueController::QueueController(std::shared_ptr<models::QueueModel> queueModel)
    : m_queueModel(queueModel)
{
}

QueueController::~QueueController() 
{
}

void QueueController::addToQueue(const models::MediaFileModel& media) 
{
    // Check for duplicates
    for (const auto& item : m_queueModel->getAllItems()) {
        if (item.getFilePath() == media.getFilePath()) {
            return; // Already in queue
        }
    }
    m_queueModel->addToEnd(media);
}

void QueueController::addToQueueNext(const models::MediaFileModel& media) 
{
    for (const auto& item : m_queueModel->getAllItems()) {
        if (item.getFilePath() == media.getFilePath()) {
            return; 
        }
    }
    m_queueModel->addNext(media);
}

void QueueController::addPlaylistToQueue(const models::PlaylistModel& playlist) 
{
    auto items = playlist.getItems();
    
    for (const auto& item : items) 
    {
        m_queueModel->addToEnd(item);
    }
}

void QueueController::addMultipleToQueue(const std::vector<models::MediaFileModel>& mediaList) 
{
    for (const auto& media : mediaList) 
    {
        bool exists = false;
        for (const auto& item : m_queueModel->getAllItems()) {
            if (item.getFilePath() == media.getFilePath()) {
                exists = true;
                break;
            }
        }
        if (!exists) {
            m_queueModel->addToEnd(media);
        }
    }
}

bool QueueController::removeFromQueue(size_t index) 
{
    if (m_queueModel->removeAt(index)) 
    {
        return true;
    }
    
    return false;
}

bool QueueController::removeByPath(const std::string& filePath) 
{
    if (m_queueModel->removeByPath(filePath)) 
    {
        return true;
    }
    
    return false;
}

void QueueController::clearQueue() 
{
    m_queueModel->clear();
}

bool QueueController::jumpToIndex(size_t index) 
{
    if (m_queueModel->jumpTo(index)) 
    {
        return true;
    }
    
    return false;
}

bool QueueController::moveToNext() 
{
    if (m_queueModel->moveToNext()) 
    {
        return true;
    }
    
    return false;
}

bool QueueController::moveToPrevious() 
{
    if (m_queueModel->moveToPrevious()) 
    {
        return true;
    }
    
    return false;
}

bool QueueController::moveItem(size_t fromIndex, size_t toIndex) 
{
    if (m_queueModel->moveItem(fromIndex, toIndex)) 
    {
        return true;
    }
    
    return false;
}

void QueueController::toggleShuffle() 
{
    bool currentShuffle = m_queueModel->isShuffleEnabled();
    m_queueModel->setShuffleMode(!currentShuffle);
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
}

void QueueController::toggleRepeat() 
{
    cycleRepeatMode();
}

void QueueController::setShuffle(bool enabled) 
{
    m_queueModel->setShuffleMode(enabled);
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
