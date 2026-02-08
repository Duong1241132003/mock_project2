#ifndef MOCK_LIBRARY_MODEL_H
#define MOCK_LIBRARY_MODEL_H

#include <gmock/gmock.h>
#include "models/LibraryModel.h"

namespace media_player {
namespace test {

class MockLibraryModel : public models::LibraryModel {
public:
    MOCK_METHOD(void, addFile, (const models::MediaFileModel& file), (override));
    MOCK_METHOD(void, removeFile, (const std::string& filePath), (override));
    MOCK_METHOD(void, clear, (), (override));
    MOCK_METHOD(std::vector<models::MediaFileModel>, getAllFiles, (), (const, override));
    MOCK_METHOD(std::vector<models::MediaFileModel>, getFilesByType, (models::MediaType type), (const, override));
    MOCK_METHOD(std::vector<models::MediaFileModel>, searchFiles, (const std::string& query), (const, override));
};

} // namespace test
} // namespace media_player

#endif // MOCK_LIBRARY_MODEL_H
