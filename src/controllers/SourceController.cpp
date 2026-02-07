// Project includes
#include "controllers/SourceController.h"
#include "utils/Logger.h"

namespace media_player 
{
namespace controllers 
{

SourceController::SourceController(
    std::shared_ptr<services::FileScanner> fileScanner,
    std::shared_ptr<repositories::LibraryRepository> libraryRepo,
    std::shared_ptr<models::LibraryModel> libraryModel
)
    : m_fileScanner(fileScanner)
    , m_libraryRepo(libraryRepo)
    , m_libraryModel(libraryModel)
    , m_monitorRunning(false)
{
    m_fileScanner->setProgressCallback(
        [this](int count, const std::string& path) 
        {
            onScanProgress(count, path);
        }
    );
    
    m_fileScanner->setCompleteCallback(
        [this](std::vector<models::MediaFileModel> results) 
        {
            onScanComplete(results);
        }
    );
    
    // Start USB monitoring -> Moved to startMonitoring()
    m_monitorRunning = false; // logic moved
    // m_usbMonitorThread = std::thread(&SourceController::monitorUsbLoop, this);
    
    LOG_INFO("SourceController initialized");
}

void SourceController::startMonitoring()
{
    if (m_monitorRunning) return;
    
    m_monitorRunning = true;
    m_usbMonitorThread = std::thread(&SourceController::monitorUsbLoop, this);
    LOG_INFO("USB Monitoring started");
}

SourceController::~SourceController() 
{
    m_monitorRunning = false;
    if (m_usbMonitorThread.joinable())
    {
        m_usbMonitorThread.join();
    }
    stopScan();
    LOG_INFO("SourceController destroyed");
}

void SourceController::selectDirectory(const std::string& path) 
{
    m_currentSourcePath = path;
    LOG_INFO("Source directory selected: " + path);
}

void SourceController::scanCurrentDirectory() 
{
    if (m_currentSourcePath.empty()) 
    {
        LOG_ERROR("No source directory selected");
        return;
    }
    
    LOG_INFO("Starting scan of: " + m_currentSourcePath);
    m_fileScanner->scanDirectory(m_currentSourcePath);
}

void SourceController::stopScan() 
{
    m_fileScanner->stopScanning();
}

void SourceController::handleUSBInserted(const std::string& mountPoint) 
{
    LOG_INFO("USB inserted at: " + mountPoint);
    
    // Notify UI instead of auto-scanning
    if (m_usbInsertedCallback) 
    {
        m_usbInsertedCallback(mountPoint);
    }
    else 
    {
        // Fallback if no callback registered (legacy behavior)
        m_currentSourcePath = mountPoint;
        scanCurrentDirectory();
    }
}

void SourceController::handleUSBRemoved() 
{
    LOG_INFO("USB removed");
    
    stopScan();
    m_currentSourcePath.clear();
}

bool SourceController::isScanning() const 
{
    return m_fileScanner->isScanning();
}

std::string SourceController::getCurrentSourcePath() const 
{
    return m_currentSourcePath;
}

void SourceController::setProgressCallback(ScanProgressCallback callback) 
{
    m_progressCallback = callback;
}

void SourceController::setCompleteCallback(ScanCompleteCallback callback) 
{
    m_completeCallback = callback;
}

void SourceController::setUsbInsertedCallback(UsbInsertedCallback callback) 
{
    m_usbInsertedCallback = callback;
}

void SourceController::onScanProgress(int count, const std::string& path) 
{
    if (m_progressCallback) 
    {
        m_progressCallback(count, path);
    }
}

void SourceController::onScanComplete(std::vector<models::MediaFileModel> results) 
{
    LOG_INFO("Scan complete. Found " + std::to_string(results.size()) + " files");
    
    // Clear old library
    m_libraryModel->clear();
    m_libraryRepo->clear();
    
    // Add to library
    m_libraryModel->addMediaBatch(results);
    m_libraryRepo->saveAll(results);
    
    if (m_completeCallback) 
    {
        m_completeCallback(results.size());
    }
}

    // Add system includes for filesystem if needed
    // #include <filesystem>  <-- Ensure this is available in the file or project

void SourceController::monitorUsbLoop() 
{
    // Simple polling for new directories in /media/duong/ or /media/
    // Adjust based on typical linux mount points. 
    // Usually USBs mount at /media/USER/LABEL
    
    std::string mediaRoot = "/media/duong"; // Adjust user name dynamically if needed, or check /media
    if (!std::filesystem::exists(mediaRoot)) 
    {
         mediaRoot = "/media";
    }
    
    std::vector<std::string> knownMounts;
    
    // Initial populate removed to trigger detection on startup
    /*
    if (std::filesystem::exists(mediaRoot)) 
    {
        for (const auto& entry : std::filesystem::directory_iterator(mediaRoot)) 
        {
            if (entry.is_directory()) 
            {
                knownMounts.push_back(entry.path().string());
            }
        }
    }
    */
    
    // Initial delay to ensure App and UI are fully ready before sending events
    std::this_thread::sleep_for(std::chrono::seconds(2));
    
    while (m_monitorRunning) 
    {
        // Check for new mounts
        if (!std::filesystem::exists(mediaRoot)) 
        {
            std::this_thread::sleep_for(std::chrono::seconds(2));
            continue;
        }
        
        std::vector<std::string> currentMounts;
        for (const auto& entry : std::filesystem::directory_iterator(mediaRoot)) 
        {
            if (entry.is_directory()) 
            {
                std::string pathObj = entry.path().string();
                // Filter out VBox guest additions
                if (pathObj.find("VBox_GAs_") != std::string::npos) {
                    continue;
                }
                currentMounts.push_back(pathObj);
            }
        }
        
        // Check for new mounts
        for (const auto& mount : currentMounts) 
        {
            bool known = false;
            for (const auto& k : knownMounts) 
            {
                if (k == mount) 
                {
                    known = true;
                    break;
                }
            }
            
            if (!known) 
            {
                // New USB detected!
                knownMounts.push_back(mount);
                // Run on main thread? No, callback is fine, but UI must be thread safe
                // ImGuiManager::showUsbPopup sets a flag, which is checked in render. 
                // Flag setting is likely not atomic but bool write usually safe enough 
                // or we trust the callback handling.
                handleUSBInserted(mount); 
            }
        }
        
        // Check for removals (optional, to update knownMounts)
        for (auto it = knownMounts.begin(); it != knownMounts.end(); ) 
        {
            bool exists = false;
            for (const auto& c : currentMounts) 
            {
                if (c == *it) 
                {
                    exists = true;
                    break;
                }
            }
            
            if (!exists) 
            {
                it = knownMounts.erase(it);
                // handleUSBRemoved(); // Optional
            }
            else 
            {
                ++it;
            }
        }
    }
}

} // namespace controllers
} // namespace media_player