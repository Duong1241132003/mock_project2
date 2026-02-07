#ifndef FILE_SCANNER_H
#define FILE_SCANNER_H

// System includes
#include <string>
#include <vector>
#include <thread>
#include <atomic>
#include <memory>

// Project includes
#include "IFileScanner.h"
#include "models/MediaFileModel.h"

namespace media_player 
{
namespace services 
{

class FileScanner : public IFileScanner 
{
public:
    FileScanner();
    ~FileScanner();
    
    // IFileScanner interface implementation
    void scanDirectory(const std::string& rootPath) override;
    void stopScanning() override;
    bool isScanning() const override;
    
    // Synchronous scan (returns results directly)
    std::vector<models::MediaFileModel> scanDirectorySync(const std::string& rootPath);
    
    void setProgressCallback(ScanProgressCallback callback) override;
    void setProgressCallback(ScanProgressCallback3 callback);
    void setCompleteCallback(ScanCompleteCallback callback) override;
    
    void setMaxDepth(int depth) override;
    void setFileExtensions(const std::vector<std::string>& extensions) override;
    
private:
    void scanWorker(const std::string& rootPath);
    void scanRecursive(const std::string& dirPath, int currentDepth);
    
    bool isValidMediaFile(const std::string& filePath) const;
    bool hasValidExtension(const std::string& extension) const;
    std::string getFileExtension(const std::string& filePath) const;
    std::string toLowerCase(const std::string& str) const;
    
    void notifyProgress(int count, const std::string& currentPath);
    void notifyComplete(const std::vector<models::MediaFileModel>& results);
    
    std::vector<std::string> m_validExtensions;
    int m_maxDepth;
    
    std::atomic<bool> m_isScanning;
    std::atomic<bool> m_shouldStop;
    std::unique_ptr<std::thread> m_scanThread;
    
    std::vector<models::MediaFileModel> m_foundFiles;
    int m_scannedCount;
    int m_totalFiles;
    
    ScanProgressCallback m_progressCallback;
    ScanProgressCallback3 m_progressCallback3;
    ScanCompleteCallback m_completeCallback;
};

} // namespace services
} // namespace media_player

#endif // FILE_SCANNER_H