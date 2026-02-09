#ifndef HISTORY_MODEL_H
#define HISTORY_MODEL_H

// System includes
#include <string>
#include <vector>
#include <optional>
#include <mutex>
#include <deque>
#include <chrono>
#include <memory>

// Project includes
#include "models/MediaFileModel.h"

namespace media_player 
{

// Forward declarations
namespace repositories 
{
    class HistoryRepository;
}

namespace models 
{

/**
 * @brief Represents a single entry in the playback history.
 * 
 * Stores the media file information along with the timestamp when it was played.
 * Follows RAII principles with automatic timestamp initialization.
 */
struct HistoryEntry 
{
    MediaFileModel media;                               ///< The media file that was played
    std::chrono::system_clock::time_point playedAt;     ///< Timestamp when the media was played
    
    /**
     * @brief Constructs a HistoryEntry with current timestamp.
     * @param m The media file model to store
     */
    explicit HistoryEntry(const MediaFileModel& m)
        : media(m)
        , playedAt(std::chrono::system_clock::now())
    {
    }
    
    /**
     * @brief Constructs a HistoryEntry with specified timestamp.
     * @param m The media file model to store
     * @param timestamp The timestamp when the media was played
     */
    HistoryEntry(const MediaFileModel& m, std::chrono::system_clock::time_point timestamp)
        : media(m)
        , playedAt(timestamp)
    {
    }
};

/**
 * @brief Model class for managing playback history data.
 * 
 * This class follows the Single Responsibility Principle by focusing solely
 * on history data management. It provides thread-safe operations for
 * adding, removing, and querying history entries.
 * 
 * The class uses RAII for resource management and delegates persistence
 * operations to HistoryRepository.
 */
class HistoryModel 
{
public:
    /**
     * @brief Constructs a HistoryModel with optional repository for persistence.
     * @param repository Shared pointer to HistoryRepository (can be nullptr for in-memory only)
     * @param maxEntries Maximum number of entries to keep (default: 100)
     */
    explicit HistoryModel(
        std::shared_ptr<repositories::HistoryRepository> repository = nullptr,
        size_t maxEntries = 100
    );
    
    /**
     * @brief Destructor - follows RAII, no manual cleanup needed.
     */
    ~HistoryModel();
    
    // ==================== Entry Operations ====================
    
    /**
     * @brief Adds a new entry to the front of the history.
     * @param media The media file to add to history
     * 
     * New entries are added to the front (most recent first).
     * If the history exceeds maxEntries, the oldest entry is removed.
     */
    void addEntry(const MediaFileModel& media);
    
    /**
     * @brief Removes the most recent entry matching the given file path.
     * @param filePath Path of the media file to remove
     * @return true if an entry was removed, false otherwise
     */
    bool removeMostRecentEntry(const std::string& filePath);
    
    /**
     * @brief Removes all entries matching the given file path.
     * @param filePath Path of the media file to remove
     * @return Number of entries removed
     */
    size_t removeAllEntries(const std::string& filePath);
    
    /**
     * @brief Clears all history entries.
     */
    void clear();
    
    // ==================== Query Operations ====================
    
    /**
     * @brief Gets the most recent N entries from history.
     * @param count Maximum number of entries to retrieve
     * @return Vector of history entries (most recent first)
     */
    std::vector<HistoryEntry> getRecentHistory(size_t count) const;
    
    /**
     * @brief Gets all entries in the history.
     * @return Vector of all history entries (most recent first)
     */
    std::vector<HistoryEntry> getAllHistory() const;
    
    /**
     * @brief Gets the entry at the specified index.
     * @param index Zero-based index (0 = most recent)
     * @return Optional containing the entry, or nullopt if index is out of range
     */
    std::optional<HistoryEntry> getEntryAt(size_t index) const;
    
    /**
     * @brief Gets the most recently played entry.
     * @return Optional containing the entry, or nullopt if history is empty
     */
    std::optional<HistoryEntry> getLastPlayed() const;
    
    /**
     * @brief Gets the second most recently played entry (for "play previous").
     * @return Optional containing the entry, or nullopt if not available
     */
    std::optional<HistoryEntry> getPreviousPlayed() const;
    
    /**
     * @brief Checks if a file was recently played.
     * @param filePath Path of the media file to check
     * @param withinMinutes Time window in minutes (default: 30)
     * @return true if the file was played within the specified time window
     */
    bool wasRecentlyPlayed(const std::string& filePath, int withinMinutes = 30) const;
    
    // ==================== State Queries ====================
    
    /**
     * @brief Gets the current number of entries in history.
     * @return Number of entries
     */
    size_t count() const;
    
    /**
     * @brief Checks if the history is empty.
     * @return true if history has no entries
     */
    bool isEmpty() const;
    
    /**
     * @brief Gets the maximum number of entries allowed.
     * @return Maximum entries limit
     */
    size_t getMaxEntries() const;
    
    // ==================== Persistence ====================
    
    /**
     * @brief Loads history from persistent storage.
     * @return true if load was successful
     */
    bool loadFromRepository();
    
    /**
     * @brief Saves history to persistent storage.
     * @return true if save was successful
     */
    bool saveToRepository();

private:
    std::shared_ptr<repositories::HistoryRepository> m_repository;  ///< Repository for persistence
    std::deque<HistoryEntry> m_history;                             ///< In-memory history storage
    size_t m_maxEntries;                                            ///< Maximum entries to keep
    mutable std::mutex m_mutex;                                     ///< Mutex for thread safety
};

} // namespace models
} // namespace media_player

#endif // HISTORY_MODEL_H
