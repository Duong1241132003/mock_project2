#ifndef LIBRARY_REPOSITORY_H
#define LIBRARY_REPOSITORY_H

// System includes
#include <string>
#include <vector>
#include <optional>
#include <mutex>
#include <map>

// Project includes
#include "IRepository.h"
#include "models/MediaFileModel.h"

namespace media_player 
{
namespace repositories 
{

class LibraryRepository : public IRepository<models::MediaFileModel> 
{
public:
    explicit LibraryRepository(const std::string& storagePath);
    ~LibraryRepository();
    
    // IRepository interface implementation
    bool save(const models::MediaFileModel& media) override;
    std::optional<models::MediaFileModel> findById(const std::string& id) override;
    std::vector<models::MediaFileModel> findAll() override;
    bool update(const models::MediaFileModel& media) override;
    bool remove(const std::string& id) override;
    bool exists(const std::string& id) override;
    
    bool saveAll(const std::vector<models::MediaFileModel>& mediaList) override;
    void clear() override;
    
    size_t count() const override;
    
    // Additional query methods
    std::optional<models::MediaFileModel> findByPath(const std::string& filePath);
    std::vector<models::MediaFileModel> findByType(models::MediaType type);
    std::vector<models::MediaFileModel> searchByFileName(const std::string& query);
    
    // Statistics
    size_t countByType(models::MediaType type) const;
    long long getTotalSize() const;
    
    // Persistence
    bool loadFromDisk();
    bool saveToDisk();
    
private:
    std::string getLibraryFilePath() const;
    bool serializeLibrary();
    bool deserializeLibrary();
    
    void ensureStorageDirectoryExists();
    std::string generateId(const std::string& filePath) const;
    
    std::string m_storagePath;
    std::map<std::string, models::MediaFileModel> m_cache;
    mutable std::mutex m_mutex;
};

} // namespace repositories
} // namespace media_player

#endif // LIBRARY_REPOSITORY_H