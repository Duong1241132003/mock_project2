#ifndef MAIN_CONTROLLER_H
#define MAIN_CONTROLLER_H

// System includes
#include <memory>

// Project includes
#include "PlaybackController.h"
#include "VideoController.h"
#include "SourceController.h"
#include "LibraryController.h"
#include "PlaylistController.h"
#include "QueueController.h"
#include "HardwareController.h"
#include "models/SystemStateModel.h"

namespace media_player 
{
namespace controllers 
{

enum class ScreenType 
{
    MAIN,
    LIBRARY,
    PLAYLIST,
    QUEUE,
    SCAN
};

class MainController 
{
public:
    MainController(
        std::shared_ptr<PlaybackController> playbackController,
        std::shared_ptr<SourceController> sourceController,
        std::shared_ptr<LibraryController> libraryController,
        std::shared_ptr<PlaylistController> playlistController,
        std::shared_ptr<QueueController> queueController,
        std::shared_ptr<HardwareController> hardwareController,
        std::shared_ptr<models::SystemStateModel> systemStateModel
    );
    
    ~MainController();
    
    // Navigation
    void navigateTo(ScreenType screen);
    ScreenType getCurrentScreen() const;
    
    // Application lifecycle
    bool initialize();
    void shutdown();
    
    // Event handling
    void handleGlobalKeyPress(int keyCode);
    void handleGlobalEvent(const std::string& event);
    
private:
    std::shared_ptr<PlaybackController> m_playbackController;
    std::shared_ptr<SourceController> m_sourceController;
    std::shared_ptr<LibraryController> m_libraryController;
    std::shared_ptr<PlaylistController> m_playlistController;
    std::shared_ptr<QueueController> m_queueController;
    std::shared_ptr<HardwareController> m_hardwareController;
    std::shared_ptr<models::SystemStateModel> m_systemStateModel;
    
    ScreenType m_currentScreen;
};

} // namespace controllers
} // namespace media_player

#endif // MAIN_CONTROLLER_H