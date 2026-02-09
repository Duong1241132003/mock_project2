#ifndef HISTORY_SCREEN_H
#define HISTORY_SCREEN_H

// System includes
#include <memory>
#include <vector>

// Project includes
#include "IView.h"
#include "controllers/HistoryController.h"
#include "models/HistoryModel.h"

namespace media_player 
{
namespace views 
{

/**
 * @brief View class for displaying playback history.
 * 
 * This class follows the MVC pattern - it only handles rendering and user input,
 * delegating all logic to HistoryController. Follows Single Responsibility Principle
 * by focusing solely on UI presentation.
 */
class HistoryScreen : public IView 
{
public:
    /**
     * @brief Constructs a HistoryScreen with a HistoryController.
     * @param historyController Shared pointer to the controller handling history logic
     */
    explicit HistoryScreen(
        std::shared_ptr<controllers::HistoryController> historyController
    );
    
    /**
     * @brief Destructor - follows RAII, no manual cleanup needed.
     */
    ~HistoryScreen() override;
    
    // ==================== IView Interface ====================
    
    /**
     * @brief Shows the history screen and refreshes data.
     */
    void show() override;
    
    /**
     * @brief Hides the history screen.
     */
    void hide() override;
    
    /**
     * @brief Updates the view state (called each frame).
     */
    void update() override;
    
    /**
     * @brief Checks if the view is currently visible.
     * @return true if visible
     */
    bool isVisible() const override;
    
    // ==================== Rendering ====================
    
    /**
     * @brief Renders the history screen UI.
     * @param painter Reference to ImGuiManager for drawing
     */
    void render(ui::ImGuiManager& painter) override;
    
    /**
     * @brief Handles user input events.
     * @param event SDL event to process
     * @return true if event was consumed
     */
    bool handleInput(const SDL_Event& event) override;
    
private:
    std::shared_ptr<controllers::HistoryController> m_historyController;  ///< Controller for history logic
    
    std::vector<models::HistoryEntry> m_cachedHistory;  ///< Cached history entries for rendering
    
    bool m_isVisible;       ///< Visibility state
    int m_scrollOffset;     ///< Scroll position for list
    
    /**
     * @brief Refreshes cached history data from controller.
     */
    void refreshCache();
};

} // namespace views
} // namespace media_player

#endif // HISTORY_SCREEN_H
