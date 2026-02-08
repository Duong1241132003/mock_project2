// Project includes
#include "models/LibraryModel.h"

// System includes
#include <algorithm>

namespace media_player 
{
namespace models 
{

void LibraryModel::addMedia(const MediaFileModel& media) 
{
    // Check if already exists
    for (const auto& item : m_mediaList) 
    {
        if (item.getFilePath() == media.getFilePath()) 
        {
            return;
        }
    }
    
    m_mediaList.push_back(media);
}

void LibraryModel::addMediaBatch(const std::vector<MediaFileModel>& mediaList) 
{
    for (const auto& media : mediaList) 
    {
        addMedia(media);
    }
}

bool LibraryModel::removeMedia(const std::string& filePath) 
{
    auto it = std::find_if(m_mediaList.begin(), m_mediaList.end(),
        [&filePath](const MediaFileModel& item) {
            return item.getFilePath() == filePath;
        });
    
    if (it != m_mediaList.end()) 
    {
        m_mediaList.erase(it);
        return true;
    }
    
    return false;
}


void LibraryModel::clear() 
{
    m_mediaList.clear();
}

bool LibraryModel::updateMedia(const std::string& filePath, const MediaFileModel& updatedMedia) 
{
    auto it = std::find_if(m_mediaList.begin(), m_mediaList.end(),
        [&filePath](const MediaFileModel& item) {
            return item.getFilePath() == filePath;
        });
    
    if (it != m_mediaList.end()) 
    {
        *it = updatedMedia;
        return true;
    }
    
    return false;
}

std::optional<MediaFileModel> LibraryModel::getMediaByPath(const std::string& filePath) const 
{
    for (const auto& media : m_mediaList) 
    {
        if (media.getFilePath() == filePath) 
        {
            return media;
        }
    }
    return std::nullopt;
}

std::vector<MediaFileModel> LibraryModel::search(const std::string& query) const 
{
    std::vector<MediaFileModel> results;
    
    for (const auto& media : m_mediaList) 
    {
        if (matchesQuery(media, query)) 
        {
            results.push_back(media);
        }
    }
    
    return results;
}

std::vector<MediaFileModel> LibraryModel::getSorted(SortCriteria criteria, bool ascending) const 
{
    std::vector<MediaFileModel> sorted = m_mediaList;
    
    auto comparator = [criteria, ascending](const MediaFileModel& a, const MediaFileModel& b) -> bool {
        bool result = false;
        
        switch (criteria) 
        {
            case SortCriteria::TITLE:
            case SortCriteria::FILE_NAME:
                result = a.getFileName() < b.getFileName();
                break;
            case SortCriteria::ARTIST:
            case SortCriteria::ALBUM:
            case SortCriteria::DATE_ADDED:
            default:
                result = a.getFileName() < b.getFileName();
                break;
        }
        
        return ascending ? result : !result;
    };
    
    std::sort(sorted.begin(), sorted.end(), comparator);
    
    return sorted;
}

std::vector<MediaFileModel> LibraryModel::getPage(size_t pageNumber, size_t itemsPerPage) const 
{
    std::vector<MediaFileModel> page;
    
    size_t startIndex = pageNumber * itemsPerPage;
    
    if (startIndex >= m_mediaList.size()) 
    {
        return page;
    }
    
    size_t endIndex = std::min(startIndex + itemsPerPage, m_mediaList.size());
    
    for (size_t i = startIndex; i < endIndex; i++) 
    {
        page.push_back(m_mediaList[i]);
    }
    
    return page;
}

int LibraryModel::getTotalAudioFiles() const 
{
    int count = 0;
    for (const auto& media : m_mediaList) 
    {
        if (media.isAudio()) 
        {
            count++;
        }
    }
    return count;
}

int LibraryModel::getTotalVideoFiles() const 
{
    int count = 0;
    for (const auto& media : m_mediaList) 
    {
        if (media.isVideo()) 
        {
            count++;
        }
    }
    return count;
}

long long LibraryModel::getTotalSize() const 
{
    long long totalSize = 0;
    for (const auto& media : m_mediaList) 
    {
        totalSize += media.getFileSize();
    }
    return totalSize;
}

bool LibraryModel::matchesQuery(const MediaFileModel& media, const std::string& query) const 
{
    std::string lowerQuery = query;
    std::transform(lowerQuery.begin(), lowerQuery.end(), lowerQuery.begin(), ::tolower);
    
    std::string lowerFileName = media.getFileName();
    std::transform(lowerFileName.begin(), lowerFileName.end(), lowerFileName.begin(), ::tolower);
    
    return lowerFileName.find(lowerQuery) != std::string::npos;
}

} // namespace models
} // namespace media_player
