// Project includes
#include "views/ScanScreen.h"
#include "utils/Logger.h"

namespace media_player 
{
namespace views 
{

ScanScreen::ScanScreen(std::shared_ptr<controllers::SourceController> sourceController)
    : m_sourceController(sourceController)
    , m_isVisible(false)
    , m_scannedCount(0)
{
    // Setup callbacks
    m_sourceController->setProgressCallback(
        [this](int count, const std::string& path) 
        {
            onScanProgress(count, path);
        }
    );
    
    m_sourceController->setCompleteCallback(
        [this](int totalFiles) 
        {
            onScanComplete(totalFiles);
        }
    );
    
    LOG_INFO("ScanScreen created");
}

ScanScreen::~ScanScreen() 
{
    LOG_INFO("ScanScreen destroyed");
}

void ScanScreen::show() 
{
    m_isVisible = true;
    LOG_INFO("ScanScreen shown");
}

void ScanScreen::hide() 
{
    m_isVisible = false;
    LOG_INFO("ScanScreen hidden");
}

void ScanScreen::update() 
{
    if (!m_isVisible) 
    {
        return;
    }
    
    // Display scan progress
    if (m_sourceController->isScanning()) 
    {
        LOG_DEBUG("Scanning: " + std::to_string(m_scannedCount) + " files found...");
        LOG_DEBUG("Current: " + m_currentPath);
    }
}

bool ScanScreen::isVisible() const 
{
    return m_isVisible;
}

void ScanScreen::startScan(const std::string& path) 
{
    LOG_INFO("Starting scan: " + path);
    
    m_scannedCount = 0;
    m_currentPath.clear();
    
    m_sourceController->selectDirectory(path);
    m_sourceController->scanCurrentDirectory();
}

void ScanScreen::stopScan() 
{
    LOG_INFO("Stopping scan");
    m_sourceController->stopScan();
}

void ScanScreen::onScanProgress(int count, const std::string& currentPath) 
{
    m_scannedCount = count;
    m_currentPath = currentPath;
}

void ScanScreen::onScanComplete(int totalFiles) 
{
    LOG_INFO("Scan complete! Found " + std::to_string(totalFiles) + " media files");
    m_scannedCount = totalFiles;
}

} // namespace views
} // namespace media_player