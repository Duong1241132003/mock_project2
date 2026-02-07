// Project includes
#include "views/PlaylistScreen.h"
#include "utils/Logger.h"

namespace media_player 
{
namespace views 
{

PlaylistScreen::PlaylistScreen(std::shared_ptr<controllers::PlaylistController> playlistController)
    : m_playlistController(playlistController)
    , m_selectedPlaylistIndex(-1)
    , m_isVisible(false)
{
    LOG_INFO("PlaylistScreen created");
}

PlaylistScreen::~PlaylistScreen() 
{
    LOG_INFO("PlaylistScreen destroyed");
}

void PlaylistScreen::show() 
{
    m_isVisible = true;
    refreshPlaylistList();
    LOG_INFO("PlaylistScreen shown");
}

void PlaylistScreen::hide() 
{
    m_isVisible = false;
    LOG_INFO("PlaylistScreen hidden");
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
        LOG_INFO("Playlist created: " + name);
        refreshPlaylistList();
    }
    else 
    {
        LOG_ERROR("Failed to create playlist: " + name);
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
        LOG_INFO("Playlist deleted: " + playlist.getName());
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
        LOG_INFO("Playlist renamed to: " + newName);
        refreshPlaylistList();
    }
}

void PlaylistScreen::selectPlaylist(size_t index) 
{
    if (index < m_playlists.size()) 
    {
        m_selectedPlaylistIndex = index;
        LOG_INFO("Playlist selected: " + m_playlists[index].getName());
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
    
    LOG_INFO("Playlist: " + playlist.getName());
    LOG_INFO("Items: " + std::to_string(items.size()));
    
    for (size_t i = 0; i < items.size(); ++i) 
    {
        LOG_INFO(std::to_string(i + 1) + ". " + items[i].getFileName());
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
    LOG_INFO("Play playlist: " + playlist.getName());
}

void PlaylistScreen::addPlaylistToQueue() 
{
    if (m_selectedPlaylistIndex < 0 || m_selectedPlaylistIndex >= static_cast<int>(m_playlists.size())) 
    {
        return;
    }
    
    const auto& playlist = m_playlists[m_selectedPlaylistIndex];
    
    // TODO: Add all playlist items to queue
    LOG_INFO("Add playlist to queue: " + playlist.getName());
}

void PlaylistScreen::refreshPlaylistList() 
{
    m_playlists = m_playlistController->getAllPlaylists();
}

void PlaylistScreen::displayPlaylists() 
{
    LOG_DEBUG("Displaying " + std::to_string(m_playlists.size()) + " playlists");
    
    for (size_t i = 0; i < m_playlists.size(); ++i) 
    {
        const auto& playlist = m_playlists[i];
        std::string marker = (static_cast<int>(i) == m_selectedPlaylistIndex) ? ">" : " ";
        
        LOG_DEBUG(marker + " " + std::to_string(i + 1) + ". " + 
                  playlist.getName() + " (" + 
                  std::to_string(playlist.getItemCount()) + " items)");
    }
}

} // namespace views
} // namespace media_player