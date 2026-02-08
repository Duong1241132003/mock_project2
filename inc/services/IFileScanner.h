#ifndef I_FILE_SCANNER_H
#define I_FILE_SCANNER_H

// System includes
#include <string>
#include <vector>
#include <functional>

// Project includes
#include "models/MediaFileModel.h"

namespace media_player 
{
namespace services 
{

// Callback types
using ScanProgressCallback = std::function<void(int currentCount, const std::string& currentPath)>;
using ScanProgressCallback3 = std::function<void(int currentCount, int total, const std::string& currentPath)>;
using ScanCompleteCallback = std::function<void(std::vector<models::MediaFileModel> results)>;

class IFileScanner 
{
public:
    virtual ~IFileScanner() = default;
    
    // Scanning operations
    virtual void scanDirectory(const std::string& rootPath) = 0;
    virtual void stopScanning() = 0;
    virtual bool isScanning() const = 0;
    
    // Synchronous scan
    virtual std::vector<models::MediaFileModel> scanDirectorySync(const std::string& rootPath) = 0;
    
    // Callbacks
    virtual void setProgressCallback(ScanProgressCallback callback) = 0;
    virtual void setCompleteCallback(ScanCompleteCallback callback) = 0;
    
    // Configuration
    virtual void setMaxDepth(int depth) = 0;
    virtual void setFileExtensions(const std::vector<std::string>& extensions) = 0;
};

} // namespace services
} // namespace media_player

#endif // I_FILE_SCANNER_H