#ifndef MOCK_FILE_SCANNER_H
#define MOCK_FILE_SCANNER_H

#include <gmock/gmock.h>
#include "services/IFileScanner.h"

namespace media_player {
namespace test {

class MockFileScanner : public services::IFileScanner {
public:
    MOCK_METHOD(void, scanDirectory, (const std::string& rootPath), (override));
    MOCK_METHOD(void, stopScanning, (), (override));
    MOCK_METHOD(bool, isScanning, (), (const, override));
    MOCK_METHOD(std::vector<models::MediaFileModel>, scanDirectorySync, (const std::string& rootPath), (override));
    
    MOCK_METHOD(void, setProgressCallback, (services::ScanProgressCallback callback), (override));
    MOCK_METHOD(void, setCompleteCallback, (services::ScanCompleteCallback callback), (override));
    
    MOCK_METHOD(void, setMaxDepth, (int depth), (override));
    MOCK_METHOD(void, setFileExtensions, (const std::vector<std::string>& extensions), (override));
};

} // namespace test
} // namespace media_player

#endif // MOCK_FILE_SCANNER_H
