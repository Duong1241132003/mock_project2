// Project includes
#include "repositories/PlaylistRepository.h"
#include "utils/Logger.h"

// System includes
#include <fstream>
#include <filesystem>
#include <sstream>
#include <algorithm>

namespace fs = std::filesystem;

namespace media_player 
{
namespace repositories 
{

PlaylistRepository::PlaylistRepository(const std::string& storagePath)
    : m_storagePath(storagePath)
{
    ensureStorageDirectoryExists();
    loadFromDisk();
    
    LOG_INFO("PlaylistRepository initialized with storage path: " + storagePath);
}

PlaylistRepository::~PlaylistRepository() 
{
    saveToDisk();
    LOG_INFO("PlaylistRepository destroyed");
}

bool PlaylistRepository::save(const models::PlaylistModel& playlist) 
{
    std::lock_guard<std::recursive_mutex> lock(m_mutex);
    
    try 
    {
        std::string id = playlist.getId();
        
        if (id.empty()) 
        {
            LOG_ERROR("Cannot save playlist with empty ID");
            return false;
        }
        
        // Save to cache
        m_cache[id] = playlist;
        
        // Persist to disk
        std::string filePath = getPlaylistFilePath(id);
        
        if (!serializePlaylist(playlist, filePath)) 
        {
            LOG_ERROR("Failed to serialize playlist to disk: " + id);
            return false;
        }
        
        LOG_INFO("Playlist saved: " + playlist.getName() + " (ID: " + id + ")");
        return true;
    }
    catch (const std::exception& e) 
    {
        LOG_ERROR("Exception in save: " + std::string(e.what()));
        return false;
    }
}

std::optional<models::PlaylistModel> PlaylistRepository::findById(const std::string& id) 
{
    std::lock_guard<std::recursive_mutex> lock(m_mutex);
    
    auto it = m_cache.find(id);
    
    if (it != m_cache.end()) 
    {
        return it->second;
    }
    
    LOG_DEBUG("Playlist not found: " + id);
    return std::nullopt;
}

std::vector<models::PlaylistModel> PlaylistRepository::findAll() 
{
    std::lock_guard<std::recursive_mutex> lock(m_mutex);
    
    std::vector<models::PlaylistModel> result;
    result.reserve(m_cache.size());
    
    for (const auto& pair : m_cache) 
    {
        result.push_back(pair.second);
    }
    
    return result;
}

bool PlaylistRepository::update(const models::PlaylistModel& playlist) 
{
    std::lock_guard<std::recursive_mutex> lock(m_mutex);
    
    std::string id = playlist.getId();
    
    // Check cache directly (don't call exists() which would deadlock)
    if (m_cache.find(id) == m_cache.end()) 
    {
        LOG_WARNING("Cannot update non-existent playlist: " + id);
        return false;
    }
    
    // Update cache
    m_cache[id] = playlist;
    
    // Persist to disk
    std::string filePath = getPlaylistFilePath(id);
    
    if (!serializePlaylist(playlist, filePath)) 
    {
        LOG_ERROR("Failed to update playlist on disk: " + id);
        return false;
    }
    
    LOG_INFO("Playlist updated: " + playlist.getName());
    return true;
}

bool PlaylistRepository::remove(const std::string& id) 
{
    std::lock_guard<std::recursive_mutex> lock(m_mutex);
    
    auto it = m_cache.find(id);
    
    if (it == m_cache.end()) 
    {
        LOG_WARNING("Cannot remove non-existent playlist: " + id);
        return false;
    }
    
    std::string name = it->second.getName();
    
    // Remove from cache
    m_cache.erase(it);
    
    // Remove from disk
    std::string filePath = getPlaylistFilePath(id);
    
    try 
    {
        if (fs::exists(filePath)) 
        {
            fs::remove(filePath);
        }
        
        LOG_INFO("Playlist removed: " + name + " (ID: " + id + ")");
        return true;
    }
    catch (const fs::filesystem_error& e) 
    {
        LOG_ERROR("Failed to remove playlist file: " + std::string(e.what()));
        return false;
    }
}

bool PlaylistRepository::exists(const std::string& id) 
{
    std::lock_guard<std::recursive_mutex> lock(m_mutex);
    return m_cache.find(id) != m_cache.end();
}

bool PlaylistRepository::saveAll(const std::vector<models::PlaylistModel>& playlists) 
{
    bool allSuccess = true;
    
    for (const auto& playlist : playlists) 
    {
        if (!save(playlist)) 
        {
            allSuccess = false;
        }
    }
    
    return allSuccess;
}

void PlaylistRepository::clear() 
{
    std::lock_guard<std::recursive_mutex> lock(m_mutex);
    
    // Remove all files
    try 
    {
        for (const auto& pair : m_cache) 
        {
            std::string filePath = getPlaylistFilePath(pair.first);
            
            if (fs::exists(filePath)) 
            {
                fs::remove(filePath);
            }
        }
    }
    catch (const fs::filesystem_error& e) 
    {
        LOG_ERROR("Error clearing playlists: " + std::string(e.what()));
    }
    
    m_cache.clear();
    LOG_INFO("All playlists cleared");
}

size_t PlaylistRepository::count() const 
{
    std::lock_guard<std::recursive_mutex> lock(m_mutex);
    return m_cache.size();
}

std::optional<models::PlaylistModel> PlaylistRepository::findByName(const std::string& name) 
{
    std::lock_guard<std::recursive_mutex> lock(m_mutex);
    
    for (const auto& pair : m_cache) 
    {
        if (pair.second.getName() == name) 
        {
            return pair.second;
        }
    }
    
    return std::nullopt;
}

std::vector<models::PlaylistModel> PlaylistRepository::searchByName(const std::string& query) 
{
    std::lock_guard<std::recursive_mutex> lock(m_mutex);
    
    std::vector<models::PlaylistModel> results;
    std::string lowerQuery = query;
    std::transform(lowerQuery.begin(), lowerQuery.end(), lowerQuery.begin(), ::tolower);
    
    for (const auto& pair : m_cache) 
    {
        std::string lowerName = pair.second.getName();
        std::transform(lowerName.begin(), lowerName.end(), lowerName.begin(), ::tolower);
        
        if (lowerName.find(lowerQuery) != std::string::npos) 
        {
            results.push_back(pair.second);
        }
    }
    
    return results;
}

bool PlaylistRepository::loadFromDisk() 
{
    std::lock_guard<std::recursive_mutex> lock(m_mutex);
    
    try 
    {
        if (!fs::exists(m_storagePath)) 
        {
            LOG_INFO("Storage path does not exist, creating: " + m_storagePath);
            ensureStorageDirectoryExists();
            return true;
        }
        
        int loadedCount = 0;
        
        for (const auto& entry : fs::directory_iterator(m_storagePath)) 
        {
            if (entry.is_regular_file() && entry.path().extension() == ".playlist") 
            {
                auto playlist = deserializePlaylist(entry.path().string());
                
                if (playlist) 
                {
                    m_cache[playlist->getId()] = *playlist;
                    loadedCount++;
                }
            }
        }
        
        LOG_INFO("Loaded " + std::to_string(loadedCount) + " playlists from disk");
        return true;
    }
    catch (const fs::filesystem_error& e) 
    {
        LOG_ERROR("Error loading playlists from disk: " + std::string(e.what()));
        return false;
    }
}

bool PlaylistRepository::saveToDisk() 
{
    std::lock_guard<std::recursive_mutex> lock(m_mutex);
    
    try 
    {
        ensureStorageDirectoryExists();
        
        for (const auto& pair : m_cache) 
        {
            std::string filePath = getPlaylistFilePath(pair.first);
            serializePlaylist(pair.second, filePath);
        }
        
        LOG_INFO("Saved " + std::to_string(m_cache.size()) + " playlists to disk");
        return true;
    }
    catch (const std::exception& e) 
    {
        LOG_ERROR("Error saving playlists to disk: " + std::string(e.what()));
        return false;
    }
}

std::string PlaylistRepository::getPlaylistFilePath(const std::string& id) const 
{
    return m_storagePath + "/" + id + ".playlist";
}

bool PlaylistRepository::serializePlaylist(const models::PlaylistModel& playlist, 
                                          const std::string& filePath) 
{
    try 
    {
        std::ofstream file(filePath);
        
        if (!file.is_open()) 
        {
            LOG_ERROR("Failed to open file for writing: " + filePath);
            return false;
        }
        
        // Write playlist metadata
        file << "ID:" << playlist.getId() << "\n";
        file << "NAME:" << playlist.getName() << "\n";
        file << "COUNT:" << playlist.getItemCount() << "\n";
        file << "ITEMS:\n";
        
        const char tab = '\t';
        for (const auto& item : playlist.getItems()) 
        {
            std::string title = item.getTitle();
            std::string artist = item.getArtist();
            for (char& c : title) if (c == tab || c == '\n' || c == '\r') c = ' ';
            for (char& c : artist) if (c == tab || c == '\n' || c == '\r') c = ' ';
            file << item.getFilePath() << tab << title << tab << artist << "\n";
        }
        
        file.close();
        return true;
    }
    catch (const std::exception& e) 
    {
        LOG_ERROR("Exception serializing playlist: " + std::string(e.what()));
        return false;
    }
}

std::optional<models::PlaylistModel> PlaylistRepository::deserializePlaylist(const std::string& filePath) 
{
    try 
    {
        std::ifstream file(filePath);
        
        if (!file.is_open()) 
        {
            LOG_ERROR("Failed to open file for reading: " + filePath);
            return std::nullopt;
        }
        
        std::string line;
        std::string id, name;
        struct ItemLine { std::string path; std::string title; std::string artist; };
        std::vector<ItemLine> itemLines;
        
        bool readingItems = false;
        
        while (std::getline(file, line)) 
        {
            if (line.empty()) 
            {
                continue;
            }
            
            if (line == "ITEMS:") 
            {
                readingItems = true;
                continue;
            }
            
            if (readingItems) 
            {
                ItemLine il;
                size_t t1 = line.find('\t');
                if (t1 != std::string::npos) {
                    il.path = line.substr(0, t1);
                    size_t t2 = line.find('\t', t1 + 1);
                    if (t2 != std::string::npos) {
                        il.title = line.substr(t1 + 1, t2 - (t1 + 1));
                        il.artist = line.substr(t2 + 1);
                    } else {
                        il.title = line.substr(t1 + 1);
                    }
                } else {
                    il.path = line;
                }
                itemLines.push_back(il);
            }
            else 
            {
                size_t colonPos = line.find(':');
                
                if (colonPos != std::string::npos) 
                {
                    std::string key = line.substr(0, colonPos);
                    std::string value = line.substr(colonPos + 1);
                    
                    if (key == "ID") 
                    {
                        id = value;
                    }
                    else if (key == "NAME") 
                    {
                        name = value;
                    }
                }
            }
        }
        
        file.close();
        
        if (id.empty() || name.empty()) 
        {
            LOG_ERROR("Invalid playlist file format: " + filePath);
            return std::nullopt;
        }
        
        // Create playlist with original ID (not a new one)
        models::PlaylistModel playlist(name);
        playlist.setId(id);  // Preserve original ID from file
        
        // Add items
        for (const auto& il : itemLines) 
        {
            if (il.path.empty()) continue;
            models::MediaFileModel media(il.path);
            if (media.isValid()) 
            {
                if (!il.title.empty()) media.setTitle(il.title);
                if (!il.artist.empty()) media.setArtist(il.artist);
                playlist.addItem(media);
            }
        }
        
        return playlist;
    }
    catch (const std::exception& e) 
    {
        LOG_ERROR("Exception deserializing playlist: " + std::string(e.what()));
        return std::nullopt;
    }
}

void PlaylistRepository::ensureStorageDirectoryExists() 
{
    try 
    {
        if (!fs::exists(m_storagePath)) 
        {
            fs::create_directories(m_storagePath);
            LOG_INFO("Created storage directory: " + m_storagePath);
        }
    }
    catch (const fs::filesystem_error& e) 
    {
        LOG_ERROR("Failed to create storage directory: " + std::string(e.what()));
    }
}

} // namespace repositories
} // namespace media_player