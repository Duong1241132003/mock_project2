// Project includes
#include "repositories/HistoryRepository.h"
#include "utils/Logger.h"

// System includes
#include <fstream>
#include <filesystem>
#include <algorithm>
#include <chrono>

namespace fs = std::filesystem;

namespace media_player 
{
namespace repositories 
{

HistoryRepository::HistoryRepository(const std::string& storagePath, size_t maxEntries)
    : m_storagePath(storagePath)
    , m_maxEntries(maxEntries)
{
    // History is in-memory only; no load from disk
    LOG_INFO("HistoryRepository initialized (max entries: " + std::to_string(maxEntries) + ", in-memory only)");
}

HistoryRepository::~HistoryRepository() 
{
    // No save to disk; history is session-only
    LOG_INFO("HistoryRepository destroyed");
}

void HistoryRepository::addEntry(const models::MediaFileModel& media) 
{
    std::lock_guard<std::mutex> lock(m_mutex);
    
    // Add to front of deque
    m_history.push_front(PlaybackHistoryEntry(media));
    
    // Maintain max size
    if (m_history.size() > m_maxEntries) 
    {
        m_history.pop_back();
    }
    
    LOG_DEBUG("Added to playback history: " + media.getFileName());
}

void HistoryRepository::removeMostRecentEntryByFilePath(const std::string& filePath)
{
    std::lock_guard<std::mutex> lock(m_mutex);
    auto it = std::find_if(m_history.begin(), m_history.end(),
        [&filePath](const PlaybackHistoryEntry& e) { return e.media.getFilePath() == filePath; });
    if (it != m_history.end())
    {
        m_history.erase(it);
        LOG_DEBUG("Removed from playback history (play previous): " + filePath);
    }
}

void HistoryRepository::removeAllEntriesByFilePath(const std::string& filePath)
{
    std::lock_guard<std::mutex> lock(m_mutex);
    auto it = std::remove_if(m_history.begin(), m_history.end(),
        [&filePath](const PlaybackHistoryEntry& e) { return e.media.getFilePath() == filePath; });
    if (it != m_history.end())
    {
        m_history.erase(it, m_history.end());
        LOG_DEBUG("Removed all history entries for: " + filePath);
    }
}

std::vector<PlaybackHistoryEntry> HistoryRepository::getRecentHistory(size_t count) const 
{
    std::lock_guard<std::mutex> lock(m_mutex);
    
    std::vector<PlaybackHistoryEntry> result;
    size_t actualCount = std::min(count, m_history.size());
    
    result.reserve(actualCount);
    
    for (size_t i = 0; i < actualCount; ++i) 
    {
        result.push_back(m_history[i]);
    }
    
    return result;
}

std::vector<PlaybackHistoryEntry> HistoryRepository::getAllHistory() const 
{
    std::lock_guard<std::mutex> lock(m_mutex);
    
    return std::vector<PlaybackHistoryEntry>(m_history.begin(), m_history.end());
}

void HistoryRepository::clear() 
{
    std::lock_guard<std::mutex> lock(m_mutex);
    
    m_history.clear();
    LOG_INFO("Playback history cleared");
}

size_t HistoryRepository::count() const 
{
    std::lock_guard<std::mutex> lock(m_mutex);
    return m_history.size();
}

bool HistoryRepository::wasRecentlyPlayed(const std::string& filePath, int withinMinutes) const 
{
    std::lock_guard<std::mutex> lock(m_mutex);
    
    auto now = std::chrono::system_clock::now();
    auto threshold = now - std::chrono::minutes(withinMinutes);
    
    for (const auto& entry : m_history) 
    {
        if (entry.media.getFilePath() == filePath && entry.playedAt >= threshold) 
        {
            return true;
        }
    }
    
    return false;
}

std::optional<PlaybackHistoryEntry> HistoryRepository::getLastPlayed() const 
{
    std::lock_guard<std::mutex> lock(m_mutex);
    
    if (m_history.empty()) 
    {
        return std::nullopt;
    }
    
    return m_history.front();
}

std::optional<PlaybackHistoryEntry> HistoryRepository::getPreviousPlayed() const
{
    std::lock_guard<std::mutex> lock(m_mutex);
    if (m_history.size() < 2)
        return std::nullopt;
    return m_history[1];
}

std::optional<PlaybackHistoryEntry> HistoryRepository::getPlayedBefore(const std::string& currentFilePath) const
{
    std::lock_guard<std::mutex> lock(m_mutex);
    for (size_t i = 0; i < m_history.size(); ++i)
        if (m_history[i].media.getFilePath() == currentFilePath && i + 1 < m_history.size())
            return m_history[i + 1];
    return std::nullopt;
}

bool HistoryRepository::loadFromDisk() 
{
    std::lock_guard<std::mutex> lock(m_mutex);
    return deserializeHistory();
}

bool HistoryRepository::saveToDisk() 
{
    std::lock_guard<std::mutex> lock(m_mutex);
    return serializeHistory();
}

bool HistoryRepository::serializeHistory() 
{
    try 
    {
        std::string filePath = getHistoryFilePath();
        std::ofstream file(filePath);
        
        if (!file.is_open()) 
        {
            LOG_ERROR("Failed to open history file for writing: " + filePath);
            return false;
        }
        
        file << "HISTORY_VERSION:1.1\n";
        file << "COUNT:" << m_history.size() << "\n";
        file << "ENTRIES:\n";
        
        const char tab = '\t';
        for (const auto& entry : m_history) 
        {
            auto timestamp = std::chrono::system_clock::to_time_t(entry.playedAt);
            std::string title = entry.media.getTitle();
            std::string artist = entry.media.getArtist();
            for (char& c : title) if (c == tab || c == '\n' || c == '\r') c = ' ';
            for (char& c : artist) if (c == tab || c == '\n' || c == '\r') c = ' ';
            file << entry.media.getFilePath() << tab << timestamp << tab << title << tab << artist << "\n";
        }
        
        file.close();
        LOG_INFO("History serialized successfully");
        
        return true;
    }
    catch (const std::exception& e) 
    {
        LOG_ERROR("Exception serializing history: " + std::string(e.what()));
        return false;
    }
}

bool HistoryRepository::deserializeHistory() 
{
    try 
    {
        std::string filePath = getHistoryFilePath();
        
        if (!fs::exists(filePath)) 
        {
            LOG_INFO("History file does not exist, starting with empty history");
            return true;
        }
        
        std::ifstream file(filePath);
        
        if (!file.is_open()) 
        {
            LOG_ERROR("Failed to open history file for reading: " + filePath);
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
                // Format v1.1: path\ttimestamp\ttitle\tartist  |  Legacy: path|timestamp
                std::string mediaPath, timestampStr, title, artist;
                if (line.find('\t') != std::string::npos) {
                    size_t a = 0, b = line.find('\t');
                    mediaPath = line.substr(a, b - a);
                    a = b + 1; b = line.find('\t', a);
                    if (b != std::string::npos) { timestampStr = line.substr(a, b - a); a = b + 1; b = line.find('\t', a); }
                    if (b != std::string::npos) { title = line.substr(a, b - a); artist = line.substr(b + 1); }
                } else {
                    size_t sep = line.find('|');
                    if (sep != std::string::npos) {
                        mediaPath = line.substr(0, sep);
                        timestampStr = line.substr(sep + 1);
                    }
                }
                if (!mediaPath.empty()) 
                {
                    models::MediaFileModel media(mediaPath);
                    if (media.isValid()) 
                    {
                        if (!title.empty()) media.setTitle(title);
                        if (!artist.empty()) media.setArtist(artist);
                        PlaybackHistoryEntry entry(media);
                        try 
                        {
                            if (!timestampStr.empty()) {
                                std::time_t timestamp = std::stoll(timestampStr);
                                entry.playedAt = std::chrono::system_clock::from_time_t(timestamp);
                            }
                        }
                        catch (...) {}
                        m_history.push_back(entry);
                        loadedCount++;
                    }
                }
            }
        }
        
        file.close();
        LOG_INFO("Loaded " + std::to_string(loadedCount) + " history entries");
        
        return true;
    }
    catch (const std::exception& e) 
    {
        LOG_ERROR("Exception deserializing history: " + std::string(e.what()));
        return false;
    }
}

void HistoryRepository::ensureStorageDirectoryExists() 
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

std::string HistoryRepository::getHistoryFilePath() const 
{
    return m_storagePath + "/history.dat";
}

} // namespace repositories
} // namespace media_player