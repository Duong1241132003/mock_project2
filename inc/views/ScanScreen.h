#ifndef SCAN_SCREEN_H
#define SCAN_SCREEN_H

// System includes
#include <memory>
#include <string>

// Project includes
#include "IView.h"
#include "controllers/SourceController.h"

namespace media_player 
{
namespace views 
{

class ScanScreen : public IView 
{
public:
    explicit ScanScreen(std::shared_ptr<controllers::SourceController> sourceController);
    ~ScanScreen();
    
    // IView interface
    void show() override;
    void hide() override;
    void update() override;
    bool isVisible() const override;
    
    // Scan control
    void startScan(const std::string& path);
    void stopScan();
    
private:
    void onScanProgress(int count, const std::string& currentPath);
    void onScanComplete(int totalFiles);
    
    std::shared_ptr<controllers::SourceController> m_sourceController;
    
    bool m_isVisible;
    int m_scannedCount;
    std::string m_currentPath;
};

} // namespace views
} // namespace media_player

#endif // SCAN_SCREEN_H