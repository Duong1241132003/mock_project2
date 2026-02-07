// Project includes
#include "controllers/SourceController.h"
#include "utils/Logger.h"

// System includes
#include <algorithm>
#include <filesystem>

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
    
    // Check if this is a storage device (not S32K144)
    if (isStorageDevice(mountPoint)) 
    {
        LOG_INFO("Storage device detected, showing popup: " + mountPoint);
        // Notify UI for storage devices
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
    else 
    {
        LOG_INFO("S32K144 device detected, not showing popup: " + mountPoint);
        // For S32K144, we might want to handle differently in the future
        // For now, just log and don't show popup
    }
}

bool SourceController::isStorageDevice(const std::string& mountPoint)
{
    // Check if mount point contains S32K144 identifier
    std::string mountLower = mountPoint;
    std::transform(mountLower.begin(), mountLower.end(), mountLower.begin(), ::tolower);
    
    // S32K144 devices typically have "EVB-S32K144" in the path
    if (mountLower.find("evb-s32k144") != std::string::npos) 
    {
        return false; // This is S32K144, not storage
    }
    
    // Additional checks for storage devices:
    // 1. Check if it contains typical media directories
    // 2. Check if it's a typical USB mount pattern
    
    if (std::filesystem::exists(mountPoint)) 
    {
        try 
        {
            // Check for typical storage device indicators
            for (const auto& entry : std::filesystem::directory_iterator(mountPoint)) 
            {
                if (entry.is_directory()) 
                {
                    std::string dirName = entry.path().filename().string();
                    std::transform(dirName.begin(), dirName.end(), dirName.begin(), ::tolower);
                    
                    // Common directories in storage devices
                    if (dirName == "music" || dirName == "videos" || dirName == "photos" || 
                        dirName == "dcim" || dirName == "documents" || dirName == "audio") 
                    {
                        return true; // Likely a storage device
                    }
                }
            }
            
            // Check if there are media files directly in root
            for (const auto& entry : std::filesystem::directory_iterator(mountPoint)) 
            {
                if (entry.is_regular_file()) 
                {
                    std::string ext = entry.path().extension().string();
                    std::transform(ext.begin(), ext.end(), ext.begin(), ::tolower);
                    
                    // Common media file extensions
                    if (ext == ".mp3" || ext == ".wav" || ext == ".flac" || ext == ".m4a" ||
                        ext == ".mp4" || ext == ".avi" || ext == ".mkv" || ext == ".mov") 
                    {
                        return true; // Has media files, likely storage
                    }
                }
            }
        }
        catch (const std::filesystem::filesystem_error& e) 
        {
            LOG_WARNING("Error checking USB device: " + std::string(e.what()));
        }
    }
    
    // Default to true for non-S32K144 devices (assume storage)
    return true;
}

bool SourceController::isS32K144Device(const std::string& mountPoint)
{
    std::string mountLower = mountPoint;
    std::transform(mountLower.begin(), mountLower.end(), mountLower.begin(), ::tolower);
    
    return mountLower.find("evb-s32k144") != std::string::npos;
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