#ifndef HISTORY_CONTROLLER_H
#define HISTORY_CONTROLLER_H

// System includes
#include <memory>
#include <vector>
#include <optional>

// Project includes
#include "models/HistoryModel.h"

// Forward declarations
namespace media_player 
{
namespace controllers 
{
    class QueueController;
    class PlaybackController;
}
}

namespace media_player 
{
namespace controllers 
{

/**
 * @brief Controller for managing History feature interactions.
 * 
 * This class follows the Single Responsibility Principle by handling
 * only the coordination between HistoryModel and other controllers
 * (QueueController, PlaybackController) for user actions.
 * 
 * It acts as the intermediary between HistoryScreen (View) and
 * HistoryModel (Model), implementing the Controller part of MVC.
 */
class HistoryController 
{
public:
    /**
     * @brief Constructs a HistoryController with required dependencies.
     * @param historyModel Shared pointer to the HistoryModel
     * @param queueController Shared pointer to QueueController for adding tracks to queue
     * @param playbackController Shared pointer to PlaybackController for playback actions
     */
    HistoryController(
        std::shared_ptr<models::HistoryModel> historyModel,
        std::shared_ptr<QueueController> queueController,
        std::shared_ptr<PlaybackController> playbackController
    );
    
    /**
     * @brief Destructor - follows RAII, no manual cleanup needed.
     */
    ~HistoryController();
    
    // ==================== View Data Access ====================
    
    /**
     * @brief Gets all history entries for display in the view.
     * @return Vector of history entries (most recent first)
     */
    std::vector<models::HistoryEntry> getHistoryEntries() const;
    
    /**
     * @brief Gets recent history entries for display.
     * @param count Maximum number of entries to retrieve
     * @return Vector of history entries
     */
    std::vector<models::HistoryEntry> getRecentHistory(size_t count) const;
    
    /**
     * @brief Gets the number of history entries.
     * @return Count of entries
     */
    size_t getHistoryCount() const;
    
    /**
     * @brief Checks if history is empty.
     * @return true if no history entries exist
     */
    bool isHistoryEmpty() const;
    
    // ==================== User Actions ====================
    
    /**
     * @brief Plays a track from history at the specified index.
     * 
     * This method adds the track to the queue and starts playback.
     * 
     * @param index Zero-based index of the history entry to play
     * @return true if playback was initiated successfully
     */
    bool playFromHistory(size_t index);
    
    /**
     * @brief Adds a new entry to history.
     * @param media The media file to add to history
     */
    void addToHistory(const models::MediaFileModel& media);
    
    /**
     * @brief Clears all history entries.
     */
    void clearHistory();
    
    /**
     * @brief Refreshes history data from the model.
     * 
     * Called by View when it needs to ensure data is up-to-date.
     */
    void refreshHistory();
    
    // ==================== History Query ====================
    
    /**
     * @brief Gets the last played entry.
     * @return Optional containing the entry, or nullopt if empty
     */
    std::optional<models::HistoryEntry> getLastPlayed() const;
    
    /**
     * @brief Gets the previously played entry (for "play previous" feature).
     * @return Optional containing the entry, or nullopt if not available
     */
    std::optional<models::HistoryEntry> getPreviousPlayed() const;

private:
    std::shared_ptr<models::HistoryModel> m_historyModel;          ///< History data model
    std::shared_ptr<QueueController> m_queueController;             ///< Queue controller for adding tracks
    std::shared_ptr<PlaybackController> m_playbackController;       ///< Playback controller for playing tracks
};

} // namespace controllers
} // namespace media_player

#endif // HISTORY_CONTROLLER_H
