// Project includes
#include "repositories/HistoryRepository.h"

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
    // Load history from disk on startup
    ensureStorageDirectoryExists();
    loadFromDisk();
}

HistoryRepository::~HistoryRepository() 
{
    // Save history to disk on shutdown
    saveToDisk();
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
    
    
}

void HistoryRepository::removeMostRecentEntryByFilePath(const std::string& filePath)
{
    std::lock_guard<std::mutex> lock(m_mutex);
    auto it = std::find_if(m_history.begin(), m_history.end(),
        [&filePath](const PlaybackHistoryEntry& e) { return e.media.getFilePath() == filePath; });
    if (it != m_history.end())
    {
        m_history.erase(it);
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
    return std::vector<PlaybackHistoryEntry>(m_history.begin(), m_history.end());
}

void HistoryRepository::setHistory(const std::vector<PlaybackHistoryEntry>& history)
{
    std::lock_guard<std::mutex> lock(m_mutex);
    m_history.clear();
    for (const auto& entry : history)
    {
        m_history.push_back(entry);
    }
}

void HistoryRepository::clear() 
{
    std::lock_guard<std::mutex> lock(m_mutex);
    
    m_history.clear();
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
        
        return true;
    }
    catch (const std::exception& e) 
    {
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
            return true;
        }
        
        std::ifstream file(filePath);
        
        if (!file.is_open()) 
        {
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
        
        return true;
    }
    catch (const std::exception& e) 
    {
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
        }
    }
    catch (const fs::filesystem_error& e) 
    {
        
    }
}

std::string HistoryRepository::getHistoryFilePath() const 
{
    return m_storagePath + "/history.dat";
}

} // namespace repositories
} // namespace media_player
