// Project includes
#include "views/PlaylistScreen.h"

namespace media_player 
{
namespace views 
{

PlaylistScreen::PlaylistScreen(std::shared_ptr<controllers::PlaylistController> playlistController)
    : m_playlistController(playlistController)
    , m_selectedPlaylistIndex(-1)
    , m_isVisible(false)
{
}

PlaylistScreen::~PlaylistScreen() 
{
}

void PlaylistScreen::show() 
{
    m_isVisible = true;
    refreshPlaylistList();
}

void PlaylistScreen::hide() 
{
    m_isVisible = false;
}

void PlaylistScreen::update() 
{
    if (!m_isVisible) 
    {
        return;
    }
    
    displayPlaylists();
}

bool PlaylistScreen::isVisible() const 
{
    return m_isVisible;
}

void PlaylistScreen::createNewPlaylist(const std::string& name) 
{
    if (m_playlistController->createPlaylist(name)) 
    {
        refreshPlaylistList();
    }
    else 
    {
    }
}

void PlaylistScreen::deleteSelectedPlaylist() 
{
    if (m_selectedPlaylistIndex < 0 || m_selectedPlaylistIndex >= static_cast<int>(m_playlists.size())) 
    {
        return;
    }
    
    const auto& playlist = m_playlists[m_selectedPlaylistIndex];
    
    if (m_playlistController->deletePlaylist(playlist.getId())) 
    {
        m_selectedPlaylistIndex = -1;
        refreshPlaylistList();
    }
}

void PlaylistScreen::renameSelectedPlaylist(const std::string& newName) 
{
    if (m_selectedPlaylistIndex < 0 || m_selectedPlaylistIndex >= static_cast<int>(m_playlists.size())) 
    {
        return;
    }
    
    const auto& playlist = m_playlists[m_selectedPlaylistIndex];
    
    if (m_playlistController->renamePlaylist(playlist.getId(), newName)) 
    {
        refreshPlaylistList();
    }
}

void PlaylistScreen::selectPlaylist(size_t index) 
{
    if (index < m_playlists.size()) 
    {
        m_selectedPlaylistIndex = index;
        
    }
}

void PlaylistScreen::viewPlaylistDetails() 
{
    if (m_selectedPlaylistIndex < 0 || m_selectedPlaylistIndex >= static_cast<int>(m_playlists.size())) 
    {
        return;
    }
    
    const auto& playlist = m_playlists[m_selectedPlaylistIndex];
    auto items = m_playlistController->getPlaylistItems(playlist.getId());
    
    for (size_t i = 0; i < items.size(); ++i) 
    {
        
    }
}

void PlaylistScreen::playPlaylist() 
{
    if (m_selectedPlaylistIndex < 0 || m_selectedPlaylistIndex >= static_cast<int>(m_playlists.size())) 
    {
        return;
    }
    
    const auto& playlist = m_playlists[m_selectedPlaylistIndex];
    
    // TODO: Add playlist to queue and start playback
}

void PlaylistScreen::addPlaylistToQueue() 
{
    if (m_selectedPlaylistIndex < 0 || m_selectedPlaylistIndex >= static_cast<int>(m_playlists.size())) 
    {
        return;
    }
    
    const auto& playlist = m_playlists[m_selectedPlaylistIndex];
    
    // TODO: Add all playlist items to queue
}

void PlaylistScreen::refreshPlaylistList() 
{
    m_playlists = m_playlistController->getAllPlaylists();
}

void PlaylistScreen::displayPlaylists() 
{
    for (size_t i = 0; i < m_playlists.size(); ++i) 
    {
        const auto& playlist = m_playlists[i];
        std::string marker = (static_cast<int>(i) == m_selectedPlaylistIndex) ? ">" : " ";
        
    }
}

} // namespace views
} // namespace media_player
