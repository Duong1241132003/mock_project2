#ifndef HISTORY_SCREEN_H
#define HISTORY_SCREEN_H

// System includes
#include <memory>
#include <vector>

// Project includes
#include "IView.h"
#include "controllers/PlaybackController.h"
#include "controllers/QueueController.h"
#include "repositories/HistoryRepository.h"

namespace media_player 
{
namespace views 
{

class HistoryScreen : public IView 
{
public:
    HistoryScreen(
        std::shared_ptr<repositories::HistoryRepository> historyRepo,
        std::shared_ptr<controllers::QueueController> queueController,
        std::shared_ptr<controllers::PlaybackController> playbackController
    );
    ~HistoryScreen() override;
    
    // IView interface
    void show() override;
    void hide() override;
    void update() override;
    bool isVisible() const override;
    
    // Rendering
    void render(ui::ImGuiManager& painter) override;
    bool handleInput(const SDL_Event& event) override;
    
    void refreshHistory();
    
private:
    std::shared_ptr<repositories::HistoryRepository> m_historyRepo;
    std::shared_ptr<controllers::QueueController> m_queueController;
    std::shared_ptr<controllers::PlaybackController> m_playbackController;
    
    std::vector<repositories::PlaybackHistoryEntry> m_historyList;
    
    bool m_isVisible;
    int m_scrollOffset;
};

} // namespace views
} // namespace media_player

#endif // HISTORY_SCREEN_H
