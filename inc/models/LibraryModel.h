#ifndef LIBRARY_MODEL_H
#define LIBRARY_MODEL_H

// System includes
#include <vector>
#include <string>
#include <memory>
#include <optional>

// Project includes
#include "MediaFileModel.h"

namespace media_player 
{
namespace models 
{

enum class SortCriteria 
{
    TITLE,
    ARTIST,
    ALBUM,
    FILE_NAME,
    DATE_ADDED
};

class LibraryModel 
{
public:
    LibraryModel() = default;
    
    // Add/Remove
    void addMedia(const MediaFileModel& media);
    void addMediaBatch(const std::vector<MediaFileModel>& mediaList);
    bool removeMedia(const std::string& filePath);
    bool updateMedia(const std::string& filePath, const MediaFileModel& updatedMedia);
    void clear();
    
    // Query
    size_t getMediaCount() const 
    { 
        return m_mediaList.size(); 
    }
    
    bool isEmpty() const 
    { 
        return m_mediaList.empty(); 
    }
    
    std::vector<MediaFileModel> getAllMedia() const 
    { 
        return m_mediaList; 
    }
    
    std::optional<MediaFileModel> getMediaByPath(const std::string& filePath) const;
    
    // Filtering and sorting
    std::vector<MediaFileModel> search(const std::string& query) const;
    std::vector<MediaFileModel> getSorted(SortCriteria criteria, bool ascending = true) const;
    std::vector<MediaFileModel> getPage(size_t pageNumber, size_t itemsPerPage) const;
    
    // Statistics
    int getTotalAudioFiles() const;
    int getTotalVideoFiles() const;
    long long getTotalSize() const;
    
private:
    bool matchesQuery(const MediaFileModel& media, const std::string& query) const;
    
    std::vector<MediaFileModel> m_mediaList;
};

} // namespace models
} // namespace media_player

#endif // LIBRARY_MODEL_H