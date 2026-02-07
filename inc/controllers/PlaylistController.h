#ifndef PLAYLIST_CONTROLLER_H
#define PLAYLIST_CONTROLLER_H

// System includes
#include <memory>
#include <vector>
#include <string>

// Project includes
#include "models/PlaylistModel.h"
#include "models/MediaFileModel.h"
#include "repositories/PlaylistRepository.h"

namespace media_player 
{
namespace controllers 
{

class PlaylistController 
{
public:
    PlaylistController(std::shared_ptr<repositories::PlaylistRepository> playlistRepo);
    ~PlaylistController();
    
    // Playlist CRUD
    bool createPlaylist(const std::string& name);
    bool deletePlaylist(const std::string& playlistId);
    bool renamePlaylist(const std::string& playlistId, const std::string& newName);
    
    // Playlist queries
    std::vector<models::PlaylistModel> getAllPlaylists() const;
    std::optional<models::PlaylistModel> getPlaylistById(const std::string& playlistId) const;
    std::optional<models::PlaylistModel> getPlaylistByName(const std::string& name) const;
    
    // Playlist items
    std::vector<models::MediaFileModel> getPlaylistItems(const std::string& playlistId) const;
    bool addMediaToPlaylist(const std::string& playlistId, const models::MediaFileModel& media);
    bool removeMediaFromPlaylist(const std::string& playlistId, size_t index);
    bool moveItemInPlaylist(const std::string& playlistId, size_t fromIndex, size_t toIndex);
    
    // Statistics
    size_t getPlaylistCount() const;
    
private:
    std::shared_ptr<repositories::PlaylistRepository> m_playlistRepo;
};

} // namespace controllers
} // namespace media_player

#endif // PLAYLIST_CONTROLLER_H