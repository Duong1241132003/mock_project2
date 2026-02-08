#ifndef MOCK_LIBRARY_REPOSITORY_H
#define MOCK_LIBRARY_REPOSITORY_H

#include <gmock/gmock.h>
#include "repositories/LibraryRepository.h"

namespace media_player {
namespace test {

class MockLibraryRepository : public repositories::LibraryRepository {
public:
    MockLibraryRepository() : repositories::LibraryRepository("test_db") {}
    
    // Abstract methods from IRepository
    MOCK_METHOD(bool, save, (const models::MediaFileModel& media), (override));
    MOCK_METHOD(std::optional<models::MediaFileModel>, findById, (const std::string& id), (override));
    MOCK_METHOD(std::vector<models::MediaFileModel>, findAll, (), (override));
    MOCK_METHOD(bool, update, (const models::MediaFileModel& media), (override));
    MOCK_METHOD(bool, remove, (const std::string& id), (override));
    MOCK_METHOD(bool, exists, (const std::string& id), (override));
    MOCK_METHOD(bool, saveAll, (const std::vector<models::MediaFileModel>& mediaList), (override));
    MOCK_METHOD(void, clear, (), (override));
    MOCK_METHOD(size_t, count, (), (const, override));
    
    // Repository specific if needed, but usually interface is enough
};

} // namespace test
} // namespace media_player

#endif // MOCK_LIBRARY_REPOSITORY_H
