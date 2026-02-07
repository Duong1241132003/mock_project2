#ifndef LIBRARY_CONTROLLER_H
#define LIBRARY_CONTROLLER_H

// System includes
#include <memory>
#include <vector>
#include <string>
#include <optional>

// Project includes
#include "models/LibraryModel.h"
#include "models/MediaFileModel.h"
#include "repositories/LibraryRepository.h"
#include "services/IMetadataReader.h"
#include "models/MetadataModel.h"

namespace media_player 
{
namespace controllers 
{

class LibraryController 
{
public:
    LibraryController(
        std::shared_ptr<models::LibraryModel> libraryModel,
        std::shared_ptr<repositories::LibraryRepository> libraryRepo,
        std::shared_ptr<services::IMetadataReader> metadataReader
    );

    // Metadata
    bool updateMetadata(const models::MediaFileModel& media, const models::MetadataModel& newMetadata);
    /** Read metadata from file (for Properties dialog). Returns nullopt if unreadable (e.g. video). */
    std::optional<models::MetadataModel> readMetadata(const std::string& filePath) const;
    
    ~LibraryController();
    
    // View library
    std::vector<models::MediaFileModel> getAllMedia() const;
    std::vector<models::MediaFileModel> getPage(size_t pageNumber, size_t itemsPerPage) const;
    
    // Filtering
    std::vector<models::MediaFileModel> getAudioFiles() const;
    std::vector<models::MediaFileModel> getVideoFiles() const;
    std::vector<models::MediaFileModel> search(const std::string& query) const;
    
    // Sorting
    std::vector<models::MediaFileModel> sortByTitle(bool ascending = true) const;
    std::vector<models::MediaFileModel> sortByArtist(bool ascending = true) const;
    std::vector<models::MediaFileModel> sortByAlbum(bool ascending = true) const;
    
    // Statistics
    size_t getTotalCount() const;
    size_t getAudioCount() const;
    size_t getVideoCount() const;
    
private:
    std::shared_ptr<models::LibraryModel> m_libraryModel;
    std::shared_ptr<repositories::LibraryRepository> m_libraryRepo;
    std::shared_ptr<services::IMetadataReader> m_metadataReader;
};

} // namespace controllers
} // namespace media_player

#endif // LIBRARY_CONTROLLER_H