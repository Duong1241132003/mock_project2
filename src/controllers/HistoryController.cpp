// Project includes
#include "controllers/HistoryController.h"
#include "controllers/QueueController.h"
#include "controllers/PlaybackController.h"

namespace media_player 
{
namespace controllers 
{

// ============================================================================
// Constructor & Destructor
// ============================================================================

HistoryController::HistoryController(
    std::shared_ptr<models::HistoryModel> historyModel,
    std::shared_ptr<QueueController> queueController,
    std::shared_ptr<PlaybackController> playbackController
)
    : m_historyModel(historyModel)
    , m_queueController(queueController)
    , m_playbackController(playbackController)
{
}

HistoryController::~HistoryController() 
{
}

// ============================================================================
// View Data Access
// ============================================================================

std::vector<models::HistoryEntry> HistoryController::getHistoryEntries() const 
{
    if (!m_historyModel) 
    {
        return {};
    }
    return m_historyModel->getAllHistory();
}

std::vector<models::HistoryEntry> HistoryController::getRecentHistory(size_t count) const 
{
    if (!m_historyModel) 
    {
        return {};
    }
    return m_historyModel->getRecentHistory(count);
}

size_t HistoryController::getHistoryCount() const 
{
    if (!m_historyModel) 
    {
        return 0;
    }
    return m_historyModel->count();
}

bool HistoryController::isHistoryEmpty() const 
{
    if (!m_historyModel) 
    {
        return true;
    }
    return m_historyModel->isEmpty();
}

// ============================================================================
// User Actions
// ============================================================================

bool HistoryController::playFromHistory(size_t index) 
{
    if (!m_historyModel || !m_queueController || !m_playbackController) 
    {
        return false;
    }
    
    // Lấy entry tại index
    auto entryOpt = m_historyModel->getEntryAt(index);
    if (!entryOpt.has_value()) 
    {
        return false;
    }
    
    const auto& entry = entryOpt.value();
    
    // Thêm vào queue
    m_queueController->addToQueue(entry.media);
    
    // Play item vừa add (ở cuối queue)
    size_t queueSize = m_queueController->getQueueSize();
    if (queueSize > 0) 
    {
        m_playbackController->playItemAt(queueSize - 1);
    }
    
    return true;
}

void HistoryController::addToHistory(const models::MediaFileModel& media) 
{
    if (m_historyModel) 
    {
        m_historyModel->addEntry(media);
    }
}

void HistoryController::clearHistory() 
{
    if (m_historyModel) 
    {
        m_historyModel->clear();
    }
}

void HistoryController::refreshHistory() 
{
    // Trong implementation hiện tại, model luôn up-to-date
    // Method này để view có thể trigger refresh nếu cần
    if (m_historyModel) 
    {
        m_historyModel->loadFromRepository();
    }
}

// ============================================================================
// History Query
// ============================================================================

std::optional<models::HistoryEntry> HistoryController::getLastPlayed() const 
{
    if (!m_historyModel) 
    {
        return std::nullopt;
    }
    return m_historyModel->getLastPlayed();
}

std::optional<models::HistoryEntry> HistoryController::getPreviousPlayed() const 
{
    if (!m_historyModel) 
    {
        return std::nullopt;
    }
    return m_historyModel->getPreviousPlayed();
}

} // namespace controllers
} // namespace media_player
