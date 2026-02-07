// Project includes
#include "controllers/LibraryController.h"
#include "utils/Logger.h"

namespace media_player 
{
namespace controllers 
{

LibraryController::LibraryController(
    std::shared_ptr<models::LibraryModel> libraryModel,
    std::shared_ptr<repositories::LibraryRepository> libraryRepo,
    std::shared_ptr<services::IMetadataReader> metadataReader
)
    : m_libraryModel(libraryModel)
    , m_libraryRepo(libraryRepo)
    , m_metadataReader(metadataReader)
{
    LOG_INFO("LibraryController initialized");
}

std::optional<models::MetadataModel> LibraryController::readMetadata(const std::string& filePath) const
{
    if (!m_metadataReader || !m_metadataReader->canReadFile(filePath))
        return std::nullopt;
    auto result = m_metadataReader->readMetadata(filePath);
    if (!result)
        return std::nullopt;
    return std::optional<models::MetadataModel>(std::move(*result));
}

bool LibraryController::updateMetadata(const models::MediaFileModel& media, const models::MetadataModel& newMetadata)
{
    if (!m_metadataReader) return false;

    // 1. Write to file
    if (m_metadataReader->writeMetadata(media.getFilePath(), newMetadata)) {
        // 2. Update in-memory model
        models::MediaFileModel updatedMedia = media;
        updatedMedia.setTitle(newMetadata.getTitle());
        updatedMedia.setArtist(newMetadata.getArtist());
        updatedMedia.setAlbum(newMetadata.getAlbum());
        // updatedMedia.setYear(newMetadata.getYear()); // MediaFileModel might not have year yet? Check MediaFileModel.h
        // updatedMedia.setGenre(newMetadata.getGenre());
        
        m_libraryModel->updateMedia(media.getFilePath(), updatedMedia);
        return true;
    }
    return false;
}

LibraryController::~LibraryController() 
{
    LOG_INFO("LibraryController destroyed");
}

std::vector<models::MediaFileModel> LibraryController::getAllMedia() const 
{
    return m_libraryModel->getAllMedia();
}

std::vector<models::MediaFileModel> LibraryController::getPage(size_t pageNumber, size_t itemsPerPage) const 
{
    return m_libraryModel->getPage(pageNumber, itemsPerPage);
}

std::vector<models::MediaFileModel> LibraryController::getAudioFiles() const 
{
    return m_libraryModel->getSorted(models::SortCriteria::FILE_NAME);
}

std::vector<models::MediaFileModel> LibraryController::getVideoFiles() const 
{
    auto allMedia = m_libraryModel->getAllMedia();
    std::vector<models::MediaFileModel> videoFiles;
    
    for (const auto& media : allMedia) 
    {
        if (media.isVideo()) 
        {
            videoFiles.push_back(media);
        }
    }
    
    return videoFiles;
}

std::vector<models::MediaFileModel> LibraryController::search(const std::string& query) const 
{
    return m_libraryModel->search(query);
}

std::vector<models::MediaFileModel> LibraryController::sortByTitle(bool ascending) const 
{
    return m_libraryModel->getSorted(models::SortCriteria::TITLE, ascending);
}

std::vector<models::MediaFileModel> LibraryController::sortByArtist(bool ascending) const 
{
    return m_libraryModel->getSorted(models::SortCriteria::ARTIST, ascending);
}

std::vector<models::MediaFileModel> LibraryController::sortByAlbum(bool ascending) const 
{
    return m_libraryModel->getSorted(models::SortCriteria::ALBUM, ascending);
}

size_t LibraryController::getTotalCount() const 
{
    return m_libraryModel->getMediaCount();
}

size_t LibraryController::getAudioCount() const 
{
    return m_libraryModel->getTotalAudioFiles();
}

size_t LibraryController::getVideoCount() const 
{
    return m_libraryModel->getTotalVideoFiles();
}

} // namespace controllers
} // namespace media_player