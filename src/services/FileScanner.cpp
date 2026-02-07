// Project includes
#include "services/FileScanner.h"
#include "config/AppConfig.h"
#include "utils/Logger.h"

// TagLib includes
#include <taglib/fileref.h>
#include <taglib/tag.h>
#include <taglib/audioproperties.h>

// System includes
#include <filesystem>
#include <algorithm>

namespace fs = std::filesystem;

namespace media_player 
{
namespace services 
{

FileScanner::FileScanner()
    : m_maxDepth(config::AppConfig::MAX_SCAN_DEPTH)
    , m_isScanning(false)
    , m_shouldStop(false)
    , m_scannedCount(0)
    , m_totalFiles(0)
{
    // Initialize with default extensions
    m_validExtensions = config::AppConfig::AUDIO_EXTENSIONS;
    auto videoExts = config::AppConfig::VIDEO_EXTENSIONS;
    m_validExtensions.insert(m_validExtensions.end(), videoExts.begin(), videoExts.end());
}

FileScanner::~FileScanner() 
{
    stopScanning();
}

void FileScanner::scanDirectory(const std::string& rootPath) 
{
    if (m_isScanning) 
    {
        LOG_WARNING("Scan already in progress. Ignoring new scan request.");
        return;
    }
    
    if (!fs::exists(rootPath)) 
    {
        LOG_ERROR("Directory does not exist: " + rootPath);
        return;
    }
    
    if (!fs::is_directory(rootPath)) 
    {
        LOG_ERROR("Path is not a directory: " + rootPath);
        return;
    }
    
    LOG_INFO("Starting scan of directory: " + rootPath);
    
    m_shouldStop = false;
    m_isScanning = true;
    m_foundFiles.clear();
    m_scannedCount = 0;
    
    // Start scanning in a separate thread
    m_scanThread = std::make_unique<std::thread>(&FileScanner::scanWorker, this, rootPath);
}

void FileScanner::stopScanning() 
{
    if (!m_isScanning) 
    {
        return;
    }
    
    LOG_INFO("Stopping scan...");
    m_shouldStop = true;
    
    if (m_scanThread && m_scanThread->joinable()) 
    {
        m_scanThread->join();
    }
    
    m_isScanning = false;
    LOG_INFO("Scan stopped.");
}

bool FileScanner::isScanning() const 
{
    return m_isScanning;
}

void FileScanner::setProgressCallback(ScanProgressCallback callback) 
{
    m_progressCallback = callback;
}

void FileScanner::setProgressCallback(ScanProgressCallback3 callback) 
{
    m_progressCallback3 = callback;
}

void FileScanner::setCompleteCallback(ScanCompleteCallback callback) 
{
    m_completeCallback = callback;
}

// Synchronous scan that returns results directly
std::vector<models::MediaFileModel> FileScanner::scanDirectorySync(const std::string& rootPath) 
{
    if (!fs::exists(rootPath)) 
    {
        LOG_ERROR("Directory does not exist: " + rootPath);
        return {};
    }
    
    if (!fs::is_directory(rootPath)) 
    {
        LOG_ERROR("Path is not a directory: " + rootPath);
        return {};
    }
    
    LOG_INFO("Starting synchronous scan of: " + rootPath);
    
    m_shouldStop = false;
    m_isScanning = true;
    m_foundFiles.clear();
    m_scannedCount = 0;
    
    // Count total files first (estimate)
    int totalEstimate = 0;
    try 
    {
        for (auto& p : fs::recursive_directory_iterator(rootPath, fs::directory_options::skip_permission_denied)) 
        {
            if (p.is_regular_file()) 
            {
                std::string ext = p.path().extension().string();
                std::transform(ext.begin(), ext.end(), ext.begin(), ::tolower);
                if (hasValidExtension(ext)) 
                {
                    totalEstimate++;
                }
            }
        }
    }
    catch (...) {}
    
    m_totalFiles = totalEstimate;
    
    scanRecursive(rootPath, 0);
    
    m_isScanning = false;
    LOG_INFO("Synchronous scan completed. Found " + std::to_string(m_foundFiles.size()) + " files");
    
    return m_foundFiles;
}

void FileScanner::setMaxDepth(int depth) 
{
    m_maxDepth = depth;
}

void FileScanner::setFileExtensions(const std::vector<std::string>& extensions) 
{
    m_validExtensions = extensions;
}

void FileScanner::scanWorker(const std::string& rootPath) 
{
    try 
    {
        scanRecursive(rootPath, 0);
        
        if (!m_shouldStop) 
        {
            LOG_INFO("Scan completed. Found " + std::to_string(m_foundFiles.size()) + " media files.");
            notifyComplete(m_foundFiles);
        }
        else 
        {
            LOG_INFO("Scan was stopped by user.");
        }
    }
    catch (const std::exception& e) 
    {
        LOG_ERROR("Exception during scan: " + std::string(e.what()));
    }
    
    m_isScanning = false;
}

void FileScanner::scanRecursive(const std::string& dirPath, int currentDepth) 
{
    if (m_shouldStop) 
    {
        return;
    }
    
    if (currentDepth > m_maxDepth) 
    {
        LOG_WARNING("Max depth reached at: " + dirPath);
        return;
    }
    
    try 
    {
        for (const auto& entry : fs::directory_iterator(dirPath)) 
        {
            if (m_shouldStop) 
            {
                return;
            }
            
            try 
            {
                if (entry.is_directory()) 
                {
                    // Skip hidden directories
                    if (entry.path().filename().string()[0] == '.') 
                    {
                        continue;
                    }
                    
                    scanRecursive(entry.path().string(), currentDepth + 1);
                }
                else if (entry.is_regular_file()) 
                {
                    std::string filePath = entry.path().string();
                    
                    if (isValidMediaFile(filePath)) 
                    {
                        models::MediaFileModel media(filePath);
                        
                        if (media.isValid()) 
                        {
                            // Read metadata using TagLib
                            try 
                            {
                                TagLib::FileRef file(filePath.c_str());
                                if (!file.isNull()) 
                                {
                                    TagLib::Tag* tag = file.tag();
                                    if (tag) 
                                    {
                                        std::string title = tag->title().toCString(true);
                                        std::string artist = tag->artist().toCString(true);
                                        std::string album = tag->album().toCString(true);
                                        
                                        if (!title.empty()) media.setTitle(title);
                                        if (!artist.empty()) media.setArtist(artist);
                                        if (!album.empty()) media.setAlbum(album);
                                    }
                                    
                                    TagLib::AudioProperties* props = file.audioProperties();
                                    if (props) 
                                    {
                                        media.setDuration(props->lengthInSeconds());
                                    }
                                }
                            }
                            catch (...) 
                            {
                                // Ignore metadata read errors
                            }
                            
                            m_foundFiles.push_back(media);
                            m_scannedCount++;
                            
                            // Notify progress every 10 files
                            if (m_scannedCount % 10 == 0) 
                            {
                                notifyProgress(m_scannedCount, filePath);
                            }
                        }
                    }
                    else
                    {
                        // Log skipped file for verification
                        // LOG_DEBUG("Skipping unsupported file: " + filePath);
                    }
                }
            }
            catch (const fs::filesystem_error& e) 
            {
                LOG_WARNING("Error accessing: " + entry.path().string() + " - " + e.what());
                continue;
            }
        }
    }
    catch (const fs::filesystem_error& e) 
    {
        LOG_ERROR("Error scanning directory: " + dirPath + " - " + e.what());
    }
}

bool FileScanner::isValidMediaFile(const std::string& filePath) const 
{
    std::string extension = getFileExtension(filePath);
    return hasValidExtension(extension);
}

bool FileScanner::hasValidExtension(const std::string& extension) const 
{
    std::string lowerExt = toLowerCase(extension);
    
    return std::find(m_validExtensions.begin(), 
                     m_validExtensions.end(), 
                     lowerExt) != m_validExtensions.end();
}

std::string FileScanner::getFileExtension(const std::string& filePath) const 
{
    size_t dotPos = filePath.find_last_of('.');
    
    if (dotPos == std::string::npos) 
    {
        return "";
    }
    
    return filePath.substr(dotPos);
}

std::string FileScanner::toLowerCase(const std::string& str) const 
{
    std::string result = str;
    std::transform(result.begin(), result.end(), result.begin(), ::tolower);
    return result;
}

void FileScanner::notifyProgress(int count, const std::string& currentPath) 
{
    if (m_progressCallback) 
    {
        m_progressCallback(count, currentPath);
    }
    if (m_progressCallback3) 
    {
        m_progressCallback3(count, m_totalFiles, currentPath);
    }
}

void FileScanner::notifyComplete(const std::vector<models::MediaFileModel>& results) 
{
    if (m_completeCallback) 
    {
        m_completeCallback(results);
    }
}

} // namespace services
} // namespace media_player