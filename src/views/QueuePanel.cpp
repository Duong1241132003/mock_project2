// Project includes
#include "views/QueuePanel.h"

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
}

QueuePanel::~QueuePanel() 
{
}

void QueuePanel::show() 
{
    m_isVisible = true;
    update();
}

void QueuePanel::hide() 
{
    m_isVisible = false;
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
    
}

void QueuePanel::clearQueue() 
{
    m_queueController->clearQueue();
}

void QueuePanel::moveItemUp(size_t index) 
{
    if (index > 0) 
    {
        m_queueController->moveItem(index, index - 1);
    }
}

void QueuePanel::moveItemDown(size_t index) 
{
    if (index < m_queueModel->size() - 1) 
    {
        m_queueController->moveItem(index, index + 1);
    }
}

void QueuePanel::jumpToItem(size_t index) 
{
    if (index < m_queueModel->size()) 
    {
        // TODO: Jump to this item in queue
    }
}

void QueuePanel::toggleShuffle() 
{
    m_queueController->toggleShuffle();
    
    bool shuffleEnabled = m_queueModel->isShuffleEnabled();
}

void QueuePanel::toggleRepeat() 
{
    m_queueController->toggleRepeat();
    
    bool repeatEnabled = m_queueModel->isRepeatEnabled();
}

void QueuePanel::displayQueue() 
{
    auto items = m_queueModel->getAllItems();
    size_t currentIndex = m_queueModel->getCurrentIndex();
    
    for (size_t i = 0; i < items.size(); ++i) 
    {
        std::string marker = (i == currentIndex) ? "▶" : " ";
        
    }
    
    
    
}

} // namespace views
} // namespace media_player
