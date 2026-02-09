// Project includes
#include "models/HistoryModel.h"
#include "repositories/HistoryRepository.h"

// System includes
#include <algorithm>

namespace media_player 
{
namespace models 
{

// ============================================================================
// Constructor & Destructor
// ============================================================================

HistoryModel::HistoryModel(
    std::shared_ptr<repositories::HistoryRepository> repository,
    size_t maxEntries
)
    : m_repository(repository)
    , m_maxEntries(maxEntries)
{
    // Nếu có repository, load data từ đó khi khởi tạo
    if (m_repository) 
    {
        loadFromRepository();
    }
}

HistoryModel::~HistoryModel() 
{
    // RAII: Auto-save khi destroy nếu có repository
    if (m_repository) 
    {
        saveToRepository();
    }
}

// ============================================================================
// Entry Operations
// ============================================================================

void HistoryModel::addEntry(const MediaFileModel& media) 
{
    std::lock_guard<std::mutex> lock(m_mutex);
    
    // Thêm vào đầu deque (most recent first)
    m_history.push_front(HistoryEntry(media));
    
    // Giữ số lượng trong giới hạn maxEntries
    while (m_history.size() > m_maxEntries) 
    {
        m_history.pop_back();
    }
}

bool HistoryModel::removeMostRecentEntry(const std::string& filePath) 
{
    std::lock_guard<std::mutex> lock(m_mutex);
    
    // Tìm entry đầu tiên (most recent) khớp với filePath
    auto it = std::find_if(m_history.begin(), m_history.end(),
        [&filePath](const HistoryEntry& entry) 
        {
            return entry.media.getFilePath() == filePath;
        }
    );
    
    if (it != m_history.end()) 
    {
        m_history.erase(it);
        return true;
    }
    
    return false;
}

size_t HistoryModel::removeAllEntries(const std::string& filePath) 
{
    std::lock_guard<std::mutex> lock(m_mutex);
    
    size_t originalSize = m_history.size();
    
    // Remove tất cả entries có filePath khớp
    auto newEnd = std::remove_if(m_history.begin(), m_history.end(),
        [&filePath](const HistoryEntry& entry) 
        {
            return entry.media.getFilePath() == filePath;
        }
    );
    
    m_history.erase(newEnd, m_history.end());
    
    return originalSize - m_history.size();
}

void HistoryModel::clear() 
{
    std::lock_guard<std::mutex> lock(m_mutex);
    m_history.clear();
}

// ============================================================================
// Query Operations
// ============================================================================

std::vector<HistoryEntry> HistoryModel::getRecentHistory(size_t count) const 
{
    std::lock_guard<std::mutex> lock(m_mutex);
    
    std::vector<HistoryEntry> result;
    size_t actualCount = std::min(count, m_history.size());
    
    result.reserve(actualCount);
    
    for (size_t i = 0; i < actualCount; ++i) 
    {
        result.push_back(m_history[i]);
    }
    
    return result;
}

std::vector<HistoryEntry> HistoryModel::getAllHistory() const 
{
    std::lock_guard<std::mutex> lock(m_mutex);
    return std::vector<HistoryEntry>(m_history.begin(), m_history.end());
}

std::optional<HistoryEntry> HistoryModel::getEntryAt(size_t index) const 
{
    std::lock_guard<std::mutex> lock(m_mutex);
    
    if (index >= m_history.size()) 
    {
        return std::nullopt;
    }
    
    return m_history[index];
}

std::optional<HistoryEntry> HistoryModel::getLastPlayed() const 
{
    std::lock_guard<std::mutex> lock(m_mutex);
    
    if (m_history.empty()) 
    {
        return std::nullopt;
    }
    
    return m_history.front();
}

std::optional<HistoryEntry> HistoryModel::getPreviousPlayed() const 
{
    std::lock_guard<std::mutex> lock(m_mutex);
    
    if (m_history.size() < 2) 
    {
        return std::nullopt;
    }
    
    return m_history[1];
}

bool HistoryModel::wasRecentlyPlayed(const std::string& filePath, int withinMinutes) const 
{
    std::lock_guard<std::mutex> lock(m_mutex);
    
    auto now = std::chrono::system_clock::now();
    auto threshold = now - std::chrono::minutes(withinMinutes);
    
    for (const auto& entry : m_history) 
    {
        if (entry.media.getFilePath() == filePath && entry.playedAt >= threshold) 
        {
            return true;
        }
    }
    
    return false;
}

// ============================================================================
// State Queries
// ============================================================================

size_t HistoryModel::count() const 
{
    std::lock_guard<std::mutex> lock(m_mutex);
    return m_history.size();
}

bool HistoryModel::isEmpty() const 
{
    std::lock_guard<std::mutex> lock(m_mutex);
    return m_history.empty();
}

size_t HistoryModel::getMaxEntries() const 
{
    return m_maxEntries;
}

// ============================================================================
// Persistence
// ============================================================================

bool HistoryModel::loadFromRepository() 
{
    if (!m_repository) 
    {
        return false;
    }
    
    std::lock_guard<std::mutex> lock(m_mutex);
    
    // Load từ repository và convert sang HistoryEntry
    auto repoHistory = m_repository->getAllHistory();
    m_history.clear();
    
    for (const auto& repoEntry : repoHistory) 
    {
        m_history.push_back(HistoryEntry(repoEntry.media, repoEntry.playedAt));
    }
    
    return true;
}

bool HistoryModel::saveToRepository() 
{
    if (!m_repository) 
    {
        return false;
    }
    
    // Sync data từ model sang repository
    // Repository sẽ handle việc serialize
    
    // Create vector of PlaybackHistoryEntry from internal m_history
    std::vector<repositories::PlaybackHistoryEntry> entries;
    entries.reserve(m_history.size());
    
    // Lock mutex locally to read m_history
    {
        std::lock_guard<std::mutex> lock(m_mutex);
        for (const auto& entry : m_history)
        {
            repositories::PlaybackHistoryEntry repoEntry(entry.media);
            repoEntry.playedAt = entry.playedAt;
            entries.push_back(repoEntry);
        }
    }
    
    m_repository->setHistory(entries);
    return m_repository->saveToDisk();
}

} // namespace models
} // namespace media_player
