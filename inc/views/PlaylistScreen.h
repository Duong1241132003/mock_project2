#ifndef PLAYLIST_SCREEN_H
#define PLAYLIST_SCREEN_H

// System includes
#include <memory>
#include <vector>
#include <string>

// Project includes
#include "IView.h"
#include "controllers/PlaylistController.h"
#include "controllers/PlaybackController.h"
#include "controllers/QueueController.h"
#include "models/PlaylistModel.h"

namespace media_player 
{
namespace views 
{

class PlaylistScreen : public IView 
{
public:
    PlaylistScreen(
        std::shared_ptr<controllers::PlaylistController> playlistController,
        std::shared_ptr<controllers::PlaybackController> playbackController,
        std::shared_ptr<controllers::QueueController> queueController
    );
    ~PlaylistScreen() override;
    
    // IView interface
    void show() override;
    void hide() override;
    void update() override;
    bool isVisible() const override;
    
    // Rendering
    void render(ui::ImGuiManager& painter) override;
    bool handleInput(const SDL_Event& event) override;
    
    // Actions - handled internally or via inputs
    
private:
    void refreshPlaylistList();
    
    std::shared_ptr<controllers::PlaylistController> m_playlistController;
    std::shared_ptr<controllers::PlaybackController> m_playbackController;
    std::shared_ptr<controllers::QueueController> m_queueController; // Added
    
    std::vector<models::PlaylistModel> m_playlists;
    std::string m_selectedPlaylistId; // Replaces index for robust selection
    
    bool m_isVisible;
    
    // UI State
    bool m_showCreateDialog;
    std::string m_newPlaylistName;
    
    bool m_showRenameDialog;
    std::string m_renamePlaylistId;
    std::string m_renamePlaylistName;
};

} // namespace views
} // namespace media_player

#endif // PLAYLIST_SCREEN_H