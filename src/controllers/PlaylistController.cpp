// Project includes
#include "controllers/PlaylistController.h"
#include "utils/Logger.h"

namespace media_player 
{
namespace controllers 
{

PlaylistController::PlaylistController(std::shared_ptr<repositories::PlaylistRepository> playlistRepo)
    : m_playlistRepo(playlistRepo)
{
    LOG_INFO("PlaylistController initialized");
}

PlaylistController::~PlaylistController() 
{
    LOG_INFO("PlaylistController destroyed");
}

bool PlaylistController::createPlaylist(const std::string& name) 
{
    if (name.empty()) 
    {
        LOG_ERROR("Playlist name cannot be empty");
        return false;
    }
    
    // Check if playlist with same name exists
    if (m_playlistRepo->findByName(name)) 
    {
        LOG_ERROR("Playlist with name '" + name + "' already exists");
        return false;
    }
    
    models::PlaylistModel playlist(name);
    
    if (m_playlistRepo->save(playlist)) 
    {
        LOG_INFO("Playlist created: " + name);
        return true;
    }
    
    LOG_ERROR("Failed to create playlist: " + name);
    return false;
}

bool PlaylistController::deletePlaylist(const std::string& playlistId) 
{
    if (m_playlistRepo->remove(playlistId)) 
    {
        LOG_INFO("Playlist deleted: " + playlistId);
        return true;
    }
    
    LOG_ERROR("Failed to delete playlist: " + playlistId);
    return false;
}

bool PlaylistController::renamePlaylist(const std::string& playlistId, const std::string& newName) 
{
    auto playlistOpt = m_playlistRepo->findById(playlistId);
    
    if (!playlistOpt) 
    {
        LOG_ERROR("Playlist not found: " + playlistId);
        return false;
    }
    
    models::PlaylistModel playlist = *playlistOpt;
    playlist.setName(newName);
    
    if (m_playlistRepo->update(playlist)) 
    {
        LOG_INFO("Playlist renamed to: " + newName);
        return true;
    }
    
    LOG_ERROR("Failed to rename playlist");
    return false;
}

std::vector<models::PlaylistModel> PlaylistController::getAllPlaylists() const 
{
    return m_playlistRepo->findAll();
}

std::optional<models::PlaylistModel> PlaylistController::getPlaylistById(const std::string& playlistId) const 
{
    return m_playlistRepo->findById(playlistId);
}

std::optional<models::PlaylistModel> PlaylistController::getPlaylistByName(const std::string& name) const 
{
    return m_playlistRepo->findByName(name);
}

std::vector<models::MediaFileModel> PlaylistController::getPlaylistItems(const std::string& playlistId) const 
{
    auto playlistOpt = m_playlistRepo->findById(playlistId);
    
    if (!playlistOpt) 
    {
        LOG_ERROR("Playlist not found: " + playlistId);
        return {};
    }
    
    return playlistOpt->getItems();
}

bool PlaylistController::addMediaToPlaylist(const std::string& playlistId, const models::MediaFileModel& media) 
{
    auto playlistOpt = m_playlistRepo->findById(playlistId);
    
    if (!playlistOpt) 
    {
        LOG_ERROR("Playlist not found: " + playlistId);
        return false;
    }
    
    models::PlaylistModel playlist = *playlistOpt;
    playlist.addItem(media);
    
    if (m_playlistRepo->update(playlist)) 
    {
        LOG_INFO("Media added to playlist: " + media.getFileName());
        return true;
    }
    
    LOG_ERROR("Failed to add media to playlist");
    return false;
}

bool PlaylistController::removeMediaFromPlaylist(const std::string& playlistId, size_t index) 
{
    auto playlistOpt = m_playlistRepo->findById(playlistId);
    
    if (!playlistOpt) 
    {
        LOG_ERROR("Playlist not found: " + playlistId);
        return false;
    }
    
    models::PlaylistModel playlist = *playlistOpt;
    
    if (!playlist.removeItem(index)) 
    {
        LOG_ERROR("Failed to remove item from playlist");
        return false;
    }
    
    if (m_playlistRepo->update(playlist)) 
    {
        LOG_INFO("Media removed from playlist");
        return true;
    }
    
    LOG_ERROR("Failed to update playlist");
    return false;
}

bool PlaylistController::moveItemInPlaylist(const std::string& playlistId, size_t fromIndex, size_t toIndex) 
{
    auto playlistOpt = m_playlistRepo->findById(playlistId);
    
    if (!playlistOpt) 
    {
        LOG_ERROR("Playlist not found: " + playlistId);
        return false;
    }
    
    models::PlaylistModel playlist = *playlistOpt;
    
    if (!playlist.moveItem(fromIndex, toIndex)) 
    {
        LOG_ERROR("Failed to move item in playlist");
        return false;
    }
    
    if (m_playlistRepo->update(playlist)) 
    {
        LOG_INFO("Item moved in playlist");
        return true;
    }
    
    LOG_ERROR("Failed to update playlist");
    return false;
}

size_t PlaylistController::getPlaylistCount() const 
{
    return m_playlistRepo->count();
}

} // namespace controllers
} // namespace media_player