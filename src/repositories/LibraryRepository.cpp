// Project includes
#include "repositories/LibraryRepository.h"
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

LibraryRepository::LibraryRepository(const std::string& storagePath)
    : m_storagePath(storagePath)
{
    ensureStorageDirectoryExists();
    loadFromDisk();
    
    LOG_INFO("LibraryRepository initialized with storage path: " + storagePath);
}

LibraryRepository::~LibraryRepository() 
{
    saveToDisk();
    LOG_INFO("LibraryRepository destroyed");
}

bool LibraryRepository::save(const models::MediaFileModel& media) 
{
    std::lock_guard<std::mutex> lock(m_mutex);
    
    std::string id = generateId(media.getFilePath());
    m_cache[id] = media;
    
    LOG_DEBUG("Media saved to library: " + media.getFileName());
    return true;
}

std::optional<models::MediaFileModel> LibraryRepository::findById(const std::string& id) 
{
    std::lock_guard<std::mutex> lock(m_mutex);
    
    auto it = m_cache.find(id);
    
    if (it != m_cache.end()) 
    {
        return it->second;
    }
    
    return std::nullopt;
}

std::vector<models::MediaFileModel> LibraryRepository::findAll() 
{
    std::lock_guard<std::mutex> lock(m_mutex);
    
    std::vector<models::MediaFileModel> result;
    result.reserve(m_cache.size());
    
    for (const auto& pair : m_cache) 
    {
        result.push_back(pair.second);
    }
    
    return result;
}

bool LibraryRepository::update(const models::MediaFileModel& media) 
{
    std::lock_guard<std::mutex> lock(m_mutex);
    
    std::string id = generateId(media.getFilePath());
    
    if (m_cache.find(id) == m_cache.end()) 
    {
        LOG_WARNING("Cannot update non-existent media: " + media.getFilePath());
        return false;
    }
    
    m_cache[id] = media;
    LOG_DEBUG("Media updated in library: " + media.getFileName());
    
    return true;
}

bool LibraryRepository::remove(const std::string& id) 
{
    std::lock_guard<std::mutex> lock(m_mutex);
    
    auto it = m_cache.find(id);
    
    if (it == m_cache.end()) 
    {
        return false;
    }
    
    std::string fileName = it->second.getFileName();
    m_cache.erase(it);
    
    LOG_DEBUG("Media removed from library: " + fileName);
    return true;
}

bool LibraryRepository::exists(const std::string& id) 
{
    std::lock_guard<std::mutex> lock(m_mutex);
    return m_cache.find(id) != m_cache.end();
}

bool LibraryRepository::saveAll(const std::vector<models::MediaFileModel>& mediaList) 
{
    std::lock_guard<std::mutex> lock(m_mutex);
    
    for (const auto& media : mediaList) 
    {
        std::string id = generateId(media.getFilePath());
        m_cache[id] = media;
    }
    
    LOG_INFO("Saved " + std::to_string(mediaList.size()) + " media files to library");
    return true;
}

void LibraryRepository::clear() 
{
    std::lock_guard<std::mutex> lock(m_mutex);
    
    m_cache.clear();
    LOG_INFO("Library cleared");
}

size_t LibraryRepository::count() const 
{
    std::lock_guard<std::mutex> lock(m_mutex);
    return m_cache.size();
}

std::optional<models::MediaFileModel> LibraryRepository::findByPath(const std::string& filePath) 
{
    std::lock_guard<std::mutex> lock(m_mutex);
    
    std::string id = generateId(filePath);
    return findById(id);
}

std::vector<models::MediaFileModel> LibraryRepository::findByType(models::MediaType type) 
{
    std::lock_guard<std::mutex> lock(m_mutex);
    
    std::vector<models::MediaFileModel> result;
    
    for (const auto& pair : m_cache) 
    {
        if (pair.second.getType() == type) 
        {
            result.push_back(pair.second);
        }
    }
    
    return result;
}

std::vector<models::MediaFileModel> LibraryRepository::searchByFileName(const std::string& query) 
{
    std::lock_guard<std::mutex> lock(m_mutex);
    
    std::vector<models::MediaFileModel> results;
    std::string lowerQuery = query;
    std::transform(lowerQuery.begin(), lowerQuery.end(), lowerQuery.begin(), ::tolower);
    
    for (const auto& pair : m_cache) 
    {
        std::string lowerName = pair.second.getFileName();
        std::transform(lowerName.begin(), lowerName.end(), lowerName.begin(), ::tolower);
        
        if (lowerName.find(lowerQuery) != std::string::npos) 
        {
            results.push_back(pair.second);
        }
    }
    
    return results;
}

size_t LibraryRepository::countByType(models::MediaType type) const 
{
    std::lock_guard<std::mutex> lock(m_mutex);
    
    size_t count = 0;
    
    for (const auto& pair : m_cache) 
    {
        if (pair.second.getType() == type) 
        {
            count++;
        }
    }
    
    return count;
}

long long LibraryRepository::getTotalSize() const 
{
    std::lock_guard<std::mutex> lock(m_mutex);
    
    long long totalSize = 0;
    
    for (const auto& pair : m_cache) 
    {
        totalSize += pair.second.getFileSize();
    }
    
    return totalSize;
}

bool LibraryRepository::loadFromDisk() 
{
    std::lock_guard<std::mutex> lock(m_mutex);
    
    return deserializeLibrary();
}

bool LibraryRepository::saveToDisk() 
{
    std::lock_guard<std::mutex> lock(m_mutex);
    
    return serializeLibrary();
}

std::string LibraryRepository::getLibraryFilePath() const 
{
    return m_storagePath + "/library.dat";
}

bool LibraryRepository::serializeLibrary() 
{
    try 
    {
        std::string filePath = getLibraryFilePath();
        std::ofstream file(filePath);
        
        if (!file.is_open()) 
        {
            LOG_ERROR("Failed to open library file for writing: " + filePath);
            return false;
        }
        
        file << "LIBRARY_VERSION:1.0\n";
        file << "COUNT:" << m_cache.size() << "\n";
        file << "ENTRIES:\n";
        
        for (const auto& pair : m_cache) 
        {
            const auto& media = pair.second;
            file << media.getFilePath() << "|"
                 << media.getFileName() << "|"
                 << media.getExtension() << "|"
                 << static_cast<int>(media.getType()) << "|"
                 << media.getFileSize() << "\n";
        }
        
        file.close();
        LOG_INFO("Library serialized successfully");
        
        return true;
    }
    catch (const std::exception& e) 
    {
        LOG_ERROR("Exception serializing library: " + std::string(e.what()));
        return false;
    }
}

bool LibraryRepository::deserializeLibrary() 
{
    try 
    {
        std::string filePath = getLibraryFilePath();
        
        if (!fs::exists(filePath)) 
        {
            LOG_INFO("Library file does not exist, starting with empty library");
            return true;
        }
        
        std::ifstream file(filePath);
        
        if (!file.is_open()) 
        {
            LOG_ERROR("Failed to open library file for reading: " + filePath);
            return false;
        }
        
        std::string line;
        bool readingEntries = false;
        int loadedCount = 0;
        
        while (std::getline(file, line)) 
        {
            if (line.empty()) 
            {
                continue;
            }
            
            if (line == "ENTRIES:") 
            {
                readingEntries = true;
                continue;
            }
            
            if (readingEntries) 
            {
                // Parse entry: filepath|filename|extension|type|size
                std::istringstream iss(line);
                std::string filePath;
                
                if (std::getline(iss, filePath, '|')) 
                {
                    models::MediaFileModel media(filePath);
                    
                    if (media.isValid()) 
                    {
                        std::string id = generateId(filePath);
                        m_cache[id] = media;
                        loadedCount++;
                    }
                }
            }
        }
        
        file.close();
        LOG_INFO("Loaded " + std::to_string(loadedCount) + " media files from library");
        
        return true;
    }
    catch (const std::exception& e) 
    {
        LOG_ERROR("Exception deserializing library: " + std::string(e.what()));
        return false;
    }
}

void LibraryRepository::ensureStorageDirectoryExists() 
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

std::string LibraryRepository::generateId(const std::string& filePath) const 
{
    // Simple hash-based ID generation
    std::hash<std::string> hasher;
    size_t hash = hasher(filePath);
    
    return "media_" + std::to_string(hash);
}

} // namespace repositories
} // namespace media_player