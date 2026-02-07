#ifndef SOURCE_CONTROLLER_H
#define SOURCE_CONTROLLER_H

// System includes
#include <memory>
#include <string>
#include <functional>
#include <thread>
#include <atomic>
#include <chrono>

// Project includes
#include "services/FileScanner.h"
#include "repositories/LibraryRepository.h"
#include "models/LibraryModel.h"

namespace media_player 
{
namespace controllers 
{

using ScanProgressCallback = std::function<void(int count, const std::string& path)>;
using ScanCompleteCallback = std::function<void(int totalFiles)>;

class SourceController 
{
public:
    SourceController(
        std::shared_ptr<services::FileScanner> fileScanner,
        std::shared_ptr<repositories::LibraryRepository> libraryRepo,
        std::shared_ptr<models::LibraryModel> libraryModel
    );
    
    ~SourceController();
    
    // Source selection
    void selectDirectory(const std::string& path);
    void scanCurrentDirectory();
    void stopScan();
    
    // USB detection
    void handleUSBInserted(const std::string& mountPoint);
    void handleUSBRemoved();
    
    // Lifecycle
    void startMonitoring();
    
    // Status
    bool isScanning() const;
    std::string getCurrentSourcePath() const;
    
    // Callbacks
    void setProgressCallback(ScanProgressCallback callback);
    void setCompleteCallback(ScanCompleteCallback callback);
    
    // USB Callback
    using UsbInsertedCallback = std::function<void(const std::string& path)>;
    void setUsbInsertedCallback(UsbInsertedCallback callback);
    
private:
    void onScanProgress(int count, const std::string& path);
    void onScanComplete(std::vector<models::MediaFileModel> results);
    
    std::shared_ptr<services::FileScanner> m_fileScanner;
    std::shared_ptr<repositories::LibraryRepository> m_libraryRepo;
    std::shared_ptr<models::LibraryModel> m_libraryModel;
    
    std::string m_currentSourcePath;
    
    ScanProgressCallback m_progressCallback;
    ScanCompleteCallback m_completeCallback;
    UsbInsertedCallback m_usbInsertedCallback;
    
    // USB Polling
    std::thread m_usbMonitorThread;
    std::atomic<bool> m_monitorRunning;
    void monitorUsbLoop();
};

} // namespace controllers
} // namespace media_player

#endif // SOURCE_CONTROLLER_H