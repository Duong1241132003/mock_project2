#ifndef PLAYLIST_SCREEN_H
#define PLAYLIST_SCREEN_H

// System includes
#include <memory>
#include <vector>
#include <string>

// Project includes
#include "IView.h"
#include "controllers/PlaylistController.h"
#include "models/PlaylistModel.h"

namespace media_player 
{
namespace views 
{

class PlaylistScreen : public IView 
{
public:
    explicit PlaylistScreen(std::shared_ptr<controllers::PlaylistController> playlistController);
    ~PlaylistScreen();
    
    // IView interface
    void show() override;
    void hide() override;
    void update() override;
    bool isVisible() const override;
    
    // Playlist management
    void createNewPlaylist(const std::string& name);
    void deleteSelectedPlaylist();
    void renameSelectedPlaylist(const std::string& newName);
    
    // Playlist selection
    void selectPlaylist(size_t index);
    void viewPlaylistDetails();
    
    // Actions
    void playPlaylist();
    void addPlaylistToQueue();
    
private:
    void refreshPlaylistList();
    void displayPlaylists();
    
    std::shared_ptr<controllers::PlaylistController> m_playlistController;
    
    std::vector<models::PlaylistModel> m_playlists;
    int m_selectedPlaylistIndex;
    
    bool m_isVisible;
};

} // namespace views
} // namespace media_player

#endif // PLAYLIST_SCREEN_H