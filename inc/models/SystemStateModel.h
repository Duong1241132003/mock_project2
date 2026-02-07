#ifndef SYSTEM_STATE_MODEL_H
#define SYSTEM_STATE_MODEL_H

// System includes
#include <string>

namespace media_player 
{
namespace models 
{

enum class AppScreen 
{
    MAIN,
    LIBRARY,
    PLAYLIST,
    QUEUE,
    VIDEO_PLAYER,
    SCAN,
    SETTINGS
};

class SystemStateModel 
{
public:
    SystemStateModel() = default;
    
    // App state
    AppScreen getCurrentScreen() const 
    { 
        return m_currentScreen; 
    }
    
    void setCurrentScreen(AppScreen screen) 
    { 
        m_currentScreen = screen; 
    }
    
    // Hardware connection
    bool isHardwareConnected() const 
    { 
        return m_hardwareConnected; 
    }
    
    void setHardwareConnected(bool connected) 
    { 
        m_hardwareConnected = connected; 
    }
    
    // Scanning state
    bool isScanning() const 
    { 
        return m_isScanning; 
    }
    
    void setScanning(bool scanning) 
    { 
        m_isScanning = scanning; 
    }
    
    int getScanProgress() const 
    { 
        return m_scanProgress; 
    }
    
    void setScanProgress(int progress) 
    { 
        m_scanProgress = progress; 
    }
    
    // Source path
    std::string getCurrentSourcePath() const 
    { 
        return m_currentSourcePath; 
    }
    
    void setCurrentSourcePath(const std::string& path) 
    { 
        m_currentSourcePath = path; 
    }
    
private:
    AppScreen m_currentScreen = AppScreen::MAIN;
    bool m_hardwareConnected = false;
    bool m_isScanning = false;
    int m_scanProgress = 0;
    std::string m_currentSourcePath;
};

} // namespace models
} // namespace media_player

#endif // SYSTEM_STATE_MODEL_H