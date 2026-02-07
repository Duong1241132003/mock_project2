#ifndef HISTORY_REPOSITORY_H
#define HISTORY_REPOSITORY_H

// System includes
#include <string>
#include <vector>
#include <optional>
#include <mutex>
#include <deque>
#include <chrono>

// Project includes
#include "models/MediaFileModel.h"

namespace media_player 
{
namespace repositories 
{

struct PlaybackHistoryEntry 
{
    models::MediaFileModel media;
    std::chrono::system_clock::time_point playedAt;
    
    PlaybackHistoryEntry(const models::MediaFileModel& m)
        : media(m)
        , playedAt(std::chrono::system_clock::now())
    {
    }
};

class HistoryRepository 
{
public:
    explicit HistoryRepository(const std::string& storagePath, size_t maxEntries = 100);
    ~HistoryRepository();
    
    // History operations
    void addEntry(const models::MediaFileModel& media);
    /** Remove the most recent history entry that matches filePath (used when play previous). */
    void removeMostRecentEntryByFilePath(const std::string& filePath);
    /** Remove all history entries that match filePath (used before add to avoid duplicates). */
    void removeAllEntriesByFilePath(const std::string& filePath);
    std::vector<PlaybackHistoryEntry> getRecentHistory(size_t count) const;
    std::vector<PlaybackHistoryEntry> getAllHistory() const;
    
    void clear();
    size_t count() const;
    
    // Query
    bool wasRecentlyPlayed(const std::string& filePath, int withinMinutes = 30) const;
    std::optional<PlaybackHistoryEntry> getLastPlayed() const;
    /** Track played before the most recent one (for "play previous" from history). */
    std::optional<PlaybackHistoryEntry> getPreviousPlayed() const;
    /** Track played before the one with currentFilePath (so previous walks back through full history). */
    std::optional<PlaybackHistoryEntry> getPlayedBefore(const std::string& currentFilePath) const;
    
    // Persistence
    bool loadFromDisk();
    bool saveToDisk();
    
private:
    bool serializeHistory();
    bool deserializeHistory();
    
    void ensureStorageDirectoryExists();
    std::string getHistoryFilePath() const;
    
    std::string m_storagePath;
    std::deque<PlaybackHistoryEntry> m_history;
    size_t m_maxEntries;
    mutable std::mutex m_mutex;
};

} // namespace repositories
} // namespace media_player

#endif // HISTORY_REPOSITORY_H