// Project includes
#include "views/QueuePanel.h"
#include "utils/Logger.h"

namespace media_player 
{
namespace views 
{

QueuePanel::QueuePanel(
    std::shared_ptr<controllers::QueueController> queueController,
    std::shared_ptr<models::QueueModel> queueModel
)
    : m_queueController(queueController)
    , m_queueModel(queueModel)
    , m_isVisible(false)
{
    LOG_INFO("QueuePanel created");
}

QueuePanel::~QueuePanel() 
{
    LOG_INFO("QueuePanel destroyed");
}

void QueuePanel::show() 
{
    m_isVisible = true;
    update();
    LOG_INFO("QueuePanel shown");
}

void QueuePanel::hide() 
{
    m_isVisible = false;
    LOG_INFO("QueuePanel hidden");
}

void QueuePanel::update() 
{
    if (!m_isVisible) 
    {
        return;
    }
    
    displayQueue();
}

bool QueuePanel::isVisible() const 
{
    return m_isVisible;
}

void QueuePanel::removeSelectedItem() 
{
    size_t currentIndex = m_queueModel->getCurrentIndex();
    m_queueController->removeFromQueue(currentIndex);
    
    LOG_INFO("Item removed from queue");
}

void QueuePanel::clearQueue() 
{
    m_queueController->clearQueue();
    LOG_INFO("Queue cleared");
}

void QueuePanel::moveItemUp(size_t index) 
{
    if (index > 0) 
    {
        m_queueController->moveItem(index, index - 1);
        LOG_INFO("Item moved up in queue");
    }
}

void QueuePanel::moveItemDown(size_t index) 
{
    if (index < m_queueModel->size() - 1) 
    {
        m_queueController->moveItem(index, index + 1);
        LOG_INFO("Item moved down in queue");
    }
}

void QueuePanel::jumpToItem(size_t index) 
{
    if (index < m_queueModel->size()) 
    {
        // TODO: Jump to this item in queue
        LOG_INFO("Jumped to queue item: " + std::to_string(index));
    }
}

void QueuePanel::toggleShuffle() 
{
    m_queueController->toggleShuffle();
    
    bool shuffleEnabled = m_queueModel->isShuffleEnabled();
    LOG_INFO("Shuffle " + std::string(shuffleEnabled ? "enabled" : "disabled"));
}

void QueuePanel::toggleRepeat() 
{
    m_queueController->toggleRepeat();
    
    bool repeatEnabled = m_queueModel->isRepeatEnabled();
    LOG_INFO("Repeat " + std::string(repeatEnabled ? "enabled" : "disabled"));
}

void QueuePanel::displayQueue() 
{
    auto items = m_queueModel->getAllItems();
    size_t currentIndex = m_queueModel->getCurrentIndex();
    
    LOG_DEBUG("Queue (" + std::to_string(items.size()) + " items):");
    
    for (size_t i = 0; i < items.size(); ++i) 
    {
        std::string marker = (i == currentIndex) ? "▶" : " ";
        LOG_DEBUG(marker + " " + std::to_string(i + 1) + ". " + items[i].getFileName());
    }
    
    LOG_DEBUG("Shuffle: " + std::string(m_queueModel->isShuffleEnabled() ? "ON" : "OFF"));
    LOG_DEBUG("Repeat: " + std::string(m_queueModel->isRepeatEnabled() ? "ON" : "OFF"));
}

} // namespace views
} // namespace media_player