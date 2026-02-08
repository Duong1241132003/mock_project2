// Project includes
#include "controllers/PlaylistController.h"

namespace media_player 
{
namespace controllers 
{

PlaylistController::PlaylistController(std::shared_ptr<repositories::PlaylistRepository> playlistRepo)
    : m_playlistRepo(playlistRepo)
{
}

PlaylistController::~PlaylistController() 
{
}

bool PlaylistController::createPlaylist(const std::string& name) 
{
    if (name.empty()) 
    {
        return false;
    }
    
    // Check if playlist with same name exists
    if (m_playlistRepo->findByName(name)) 
    {
        return false;
    }
    
    models::PlaylistModel playlist(name);
    
    if (m_playlistRepo->save(playlist)) 
    {
        return true;
    }
    return false;
}

bool PlaylistController::deletePlaylist(const std::string& playlistId) 
{
    if (m_playlistRepo->remove(playlistId)) 
    {
        return true;
    }
    return false;
}

bool PlaylistController::renamePlaylist(const std::string& playlistId, const std::string& newName) 
{
    auto playlistOpt = m_playlistRepo->findById(playlistId);
    
    if (!playlistOpt) 
    {
        return false;
    }
    
    models::PlaylistModel playlist = *playlistOpt;
    playlist.setName(newName);
    
    if (m_playlistRepo->update(playlist)) 
    {
        return true;
    }
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
        return {};
    }
    
    return playlistOpt->getItems();
}

bool PlaylistController::addMediaToPlaylist(const std::string& playlistId, const models::MediaFileModel& media) 
{
    auto playlistOpt = m_playlistRepo->findById(playlistId);
    
    if (!playlistOpt) 
    {
        return false;
    }
    
    models::PlaylistModel playlist = *playlistOpt;
    playlist.addItem(media);
    
    if (m_playlistRepo->update(playlist)) 
    {
        return true;
    }
    return false;
}

bool PlaylistController::removeMediaFromPlaylist(const std::string& playlistId, size_t index) 
{
    auto playlistOpt = m_playlistRepo->findById(playlistId);
    
    if (!playlistOpt) 
    {
        return false;
    }
    
    models::PlaylistModel playlist = *playlistOpt;
    
    if (!playlist.removeItem(index)) 
    {
        return false;
    }
    
    if (m_playlistRepo->update(playlist)) 
    {
        return true;
    }
    return false;
}

bool PlaylistController::moveItemInPlaylist(const std::string& playlistId, size_t fromIndex, size_t toIndex) 
{
    auto playlistOpt = m_playlistRepo->findById(playlistId);
    
    if (!playlistOpt) 
    {
        return false;
    }
    
    models::PlaylistModel playlist = *playlistOpt;
    
    if (!playlist.moveItem(fromIndex, toIndex)) 
    {
        return false;
    }
    
    if (m_playlistRepo->update(playlist)) 
    {
        return true;
    }
    return false;
}

size_t PlaylistController::getPlaylistCount() const 
{
    return m_playlistRepo->count();
}

} // namespace controllers
} // namespace media_player
