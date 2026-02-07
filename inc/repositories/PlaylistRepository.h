#ifndef PLAYLIST_REPOSITORY_H
#define PLAYLIST_REPOSITORY_H

// System includes
#include <string>
#include <vector>
#include <optional>
#include <mutex>
#include <map>

// Project includes
#include "IRepository.h"
#include "models/PlaylistModel.h"

namespace media_player 
{
namespace repositories 
{

class PlaylistRepository : public IRepository<models::PlaylistModel> 
{
public:
    explicit PlaylistRepository(const std::string& storagePath);
    ~PlaylistRepository();
    
    // IRepository interface implementation
    bool save(const models::PlaylistModel& playlist) override;
    std::optional<models::PlaylistModel> findById(const std::string& id) override;
    std::vector<models::PlaylistModel> findAll() override;
    bool update(const models::PlaylistModel& playlist) override;
    bool remove(const std::string& id) override;
    bool exists(const std::string& id) override;
    
    bool saveAll(const std::vector<models::PlaylistModel>& playlists) override;
    void clear() override;
    
    size_t count() const override;
    
    // Additional query methods
    std::optional<models::PlaylistModel> findByName(const std::string& name);
    std::vector<models::PlaylistModel> searchByName(const std::string& query);
    
    // Persistence
    bool loadFromDisk();
    bool saveToDisk();
    
private:
    std::string getPlaylistFilePath(const std::string& id) const;
    bool serializePlaylist(const models::PlaylistModel& playlist, const std::string& filePath);
    std::optional<models::PlaylistModel> deserializePlaylist(const std::string& filePath);
    
    void ensureStorageDirectoryExists();
    
    std::string m_storagePath;
    std::map<std::string, models::PlaylistModel> m_cache;
    mutable std::recursive_mutex m_mutex;  // Use recursive_mutex to prevent deadlocks
};

} // namespace repositories
} // namespace media_player

#endif // PLAYLIST_REPOSITORY_H