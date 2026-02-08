// Project includes
#include "views/ScanScreen.h"

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
    
    
}

ScanScreen::~ScanScreen() 
{
}

void ScanScreen::show() 
{
    m_isVisible = true;
}

void ScanScreen::hide() 
{
    m_isVisible = false;
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
    }
}

bool ScanScreen::isVisible() const 
{
    return m_isVisible;
}

void ScanScreen::startScan(const std::string& path) 
{
    
    m_scannedCount = 0;
    m_currentPath.clear();
    
    m_sourceController->selectDirectory(path);
    m_sourceController->scanCurrentDirectory();
}

void ScanScreen::stopScan() 
{
    m_sourceController->stopScan();
}

void ScanScreen::onScanProgress(int count, const std::string& currentPath) 
{
    m_scannedCount = count;
    m_currentPath = currentPath;
}

void ScanScreen::onScanComplete(int totalFiles) 
{
    m_scannedCount = totalFiles;
}

} // namespace views
} // namespace media_player
