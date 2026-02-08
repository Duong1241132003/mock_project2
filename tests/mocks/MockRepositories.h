#ifndef MOCK_REPOSITORIES_H
#define MOCK_REPOSITORIES_H

#include <gmock/gmock.h>
#include "repositories/IRepository.h"
#include "models/MediaFileModel.h"
#include "models/PlaylistModel.h"

namespace media_player {
namespace test {

template<typename T>
class MockRepository : public repositories::IRepository<T> {
public:
    MOCK_METHOD(bool, save, (const T& item), (override));
    MOCK_METHOD(std::optional<T>, findById, (const std::string& id), (override));
    MOCK_METHOD(std::vector<T>, findAll, (), (override));
    MOCK_METHOD(bool, update, (const T& item), (override));
    MOCK_METHOD(bool, remove, (const std::string& id), (override));
    MOCK_METHOD(bool, exists, (const std::string& id), (override));
    
    MOCK_METHOD(bool, saveAll, (const std::vector<T>& items), (override));
    MOCK_METHOD(void, clear, (), (override));
    
    MOCK_METHOD(size_t, count, (), (const, override));
};

using MockLibraryRepository = MockRepository<models::MediaFileModel>;
using MockPlaylistRepository = MockRepository<models::PlaylistModel>;

} // namespace test
} // namespace media_player

#endif // MOCK_REPOSITORIES_H
