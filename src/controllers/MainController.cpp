// Project includes
#include "controllers/MainController.h"

namespace media_player 
{
namespace controllers 
{

MainController::MainController(
    std::shared_ptr<PlaybackController> playbackController,
    std::shared_ptr<SourceController> sourceController,
    std::shared_ptr<LibraryController> libraryController,
    std::shared_ptr<PlaylistController> playlistController,
    std::shared_ptr<QueueController> queueController,
    std::shared_ptr<HardwareController> hardwareController,
    std::shared_ptr<models::SystemStateModel> systemStateModel
)
    : m_playbackController(playbackController)
    , m_sourceController(sourceController)
    , m_libraryController(libraryController)
    , m_playlistController(playlistController)
    , m_queueController(queueController)
    , m_hardwareController(hardwareController)
    , m_systemStateModel(systemStateModel)
    , m_currentScreen(ScreenType::MAIN)
{
}

MainController::~MainController() 
{
    shutdown();
}

void MainController::navigateTo(ScreenType screen) 
{
    m_currentScreen = screen;
    
    switch (screen) 
    {
        case ScreenType::MAIN:
            break;
        case ScreenType::LIBRARY:
            break;
        case ScreenType::PLAYLIST:
            break;
        case ScreenType::QUEUE:
            break;
        case ScreenType::SCAN:
            break;
    }
}

ScreenType MainController::getCurrentScreen() const 
{
    return m_currentScreen;
}

bool MainController::initialize() 
{
    // Initialize hardware controller
    if (m_hardwareController) 
    {
        m_hardwareController->initialize();
    }
    
    return true;
}

void MainController::shutdown() 
{
    // Stop playback
    if (m_playbackController) 
    {
        m_playbackController->stop();
    }
    
    // Stop scanning
    if (m_sourceController) 
    {
        m_sourceController->stopScan();
    }
    
    // Disconnect hardware
    if (m_hardwareController) 
    {
        m_hardwareController->disconnect();
    }
    
}

void MainController::handleGlobalKeyPress(int keyCode) 
{
    // Handle global keyboard shortcuts
    (void)keyCode;
}

void MainController::handleGlobalEvent(const std::string& event) 
{
    (void)event;
}

} // namespace controllers
} // namespace media_player
