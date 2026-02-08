// Project includes
#include "services/FileScanner.h"
#include "config/AppConfig.h"

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
    // Initialize with audio and video extensions
    // Initialize with all scannable extensions
    m_validExtensions = config::AppConfig::SCANNABLE_EXTENSIONS;
}

FileScanner::~FileScanner() 
{
    stopScanning();
}

void FileScanner::scanDirectory(const std::string& rootPath) 
{
    if (m_isScanning) 
    {
        return;
    }
    
    if (!fs::exists(rootPath)) 
    {
        return;
    }
    
    if (!fs::is_directory(rootPath)) 
    {
        return;
    }
    
    
    m_shouldStop = false;
    m_isScanning = true;
    m_foundFiles.clear();
    m_scannedCount = 0;
    
    // Ensure previous thread is joined before creating new one
    if (m_scanThread && m_scanThread->joinable()) 
    {
        m_scanThread->join();
    }
    
    // Start scanning in a separate thread
    m_scanThread = std::make_unique<std::thread>(&FileScanner::scanWorker, this, rootPath);
}

void FileScanner::stopScanning() 
{
    m_shouldStop = true;
    
    if (m_scanThread && m_scanThread->joinable()) 
    {
        m_scanThread->join();
    }
    
    m_isScanning = false;
    // Reduce log spam if already stopped
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
        return {};
    }
    
    if (!fs::is_directory(rootPath)) 
    {
        return {};
    }
    
    
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
            notifyComplete(m_foundFiles);
        }
        else 
        {
        }
    }
    catch (const std::exception& e) 
    {
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
                    }
                }
            }
            catch (const fs::filesystem_error& e) 
            {
                continue;
            }
        }
    }
    catch (const fs::filesystem_error& e) 
    {
        
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
