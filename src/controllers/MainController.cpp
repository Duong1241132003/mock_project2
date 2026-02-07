// Project includes
#include "controllers/MainController.h"
#include "utils/Logger.h"

namespace media_player 
{
namespace controllers 
{

MainController::MainController(
    std::shared_ptr<PlaybackController> playbackController,
    std::shared_ptr<VideoController> videoController,
    std::shared_ptr<SourceController> sourceController,
    std::shared_ptr<LibraryController> libraryController,
    std::shared_ptr<PlaylistController> playlistController,
    std::shared_ptr<QueueController> queueController,
    std::shared_ptr<HardwareController> hardwareController,
    std::shared_ptr<models::SystemStateModel> systemStateModel
)
    : m_playbackController(playbackController)
    , m_videoController(videoController)
    , m_sourceController(sourceController)
    , m_libraryController(libraryController)
    , m_playlistController(playlistController)
    , m_queueController(queueController)
    , m_hardwareController(hardwareController)
    , m_systemStateModel(systemStateModel)
    , m_currentScreen(ScreenType::MAIN)
{
    LOG_INFO("MainController created");
}

MainController::~MainController() 
{
    shutdown();
    LOG_INFO("MainController destroyed");
}

void MainController::navigateTo(ScreenType screen) 
{
    m_currentScreen = screen;
    
    switch (screen) 
    {
        case ScreenType::MAIN:
            LOG_INFO("Navigated to Main Screen");
            break;
        case ScreenType::LIBRARY:
            LOG_INFO("Navigated to Library Screen");
            break;
        case ScreenType::PLAYLIST:
            LOG_INFO("Navigated to Playlist Screen");
            break;
        case ScreenType::QUEUE:
            LOG_INFO("Navigated to Queue Panel");
            break;
        case ScreenType::VIDEO_PLAYER:
            LOG_INFO("Navigated to Video Player Screen");
            break;
        case ScreenType::SCAN:
            LOG_INFO("Navigated to Scan Screen");
            break;
    }
}

ScreenType MainController::getCurrentScreen() const 
{
    return m_currentScreen;
}

bool MainController::initialize() 
{
    LOG_INFO("Initializing application...");
    
    // Initialize hardware controller
    if (m_hardwareController) 
    {
        m_hardwareController->initialize();
    }
    
    LOG_INFO("Application initialized successfully");
    return true;
}

void MainController::shutdown() 
{
    LOG_INFO("Shutting down application...");
    
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
    
    LOG_INFO("Application shutdown complete");
}

void MainController::handleGlobalKeyPress(int keyCode) 
{
    // Handle global keyboard shortcuts
    LOG_DEBUG("Global key press: " + std::to_string(keyCode));
}

void MainController::handleGlobalEvent(const std::string& event) 
{
    LOG_DEBUG("Global event: " + event);
}

} // namespace controllers
} // namespace media_player