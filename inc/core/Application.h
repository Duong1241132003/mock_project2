#ifndef APPLICATION_H
#define APPLICATION_H

// System includes
#include <memory>
#include <atomic>

// SDL includes
#include <SDL2/SDL.h>

// Project includes
#include "controllers/MainController.h"
#include "controllers/HardwareController.h"
#include "views/MainScreen.h"
#include "views/LibraryScreen.h"
#include "views/PlaylistScreen.h"
#include "views/QueuePanel.h"
#include "views/HistoryScreen.h"
#include "views/NowPlayingBar.h"
#include "models/LibraryModel.h"
#include "models/PlaybackStateModel.h"
#include "models/HistoryModel.h"
#include "controllers/HistoryController.h"
#include "services/FileScanner.h"
#include "ui/ImGuiManager.h"
#include "repositories/HistoryRepository.h"

namespace media_player 
{
namespace core 
{

// Custom Event Codes
const Uint32 EVENT_PLAYBACK_FINISHED = SDL_USEREVENT + 1;
const Uint32 EVENT_USB_INSERTED = SDL_USEREVENT + 2;

class Application 
{
public:
    Application();
    ~Application();
    
    // Application lifecycle
    bool initialize();
    int run();
    void quit();
    
    // Singleton access
    static Application& getInstance();
    
private:
    // Initialization helpers
    bool initializeSDL();
    bool createAllComponents();
    bool wireUpDependencies();
    
    // Main loop
    void processEvents();
    void update();
    void render();
    
    // Event handlers
    void handleKeyboardEvent(const SDL_KeyboardEvent& event);
    void handleMouseEvent(const SDL_MouseButtonEvent& event);
    void handleWindowEvent(const SDL_WindowEvent& event);
    
    // UI setup
    void setupUICallbacks();
    
    // Library path & scan (used by UI callbacks)
    void startScan(const std::string& path);
    void resetAndRescan(const std::string& path);
    
    // Controllers (shared ownership)
    std::shared_ptr<controllers::MainController> m_mainController;
    std::shared_ptr<controllers::PlaybackController> m_playbackController;
    std::shared_ptr<controllers::SourceController> m_sourceController;
    std::shared_ptr<controllers::LibraryController> m_libraryController;
    std::shared_ptr<controllers::PlaylistController> m_playlistController;
    std::shared_ptr<controllers::QueueController> m_queueController;
    std::shared_ptr<controllers::HistoryController> m_historyController;
    
    // Models for UI access
    std::shared_ptr<models::LibraryModel> m_libraryModel;
    std::shared_ptr<models::PlaybackStateModel> m_playbackStateModel;
    std::shared_ptr<models::HistoryModel> m_historyModel;

    std::shared_ptr<services::FileScanner> m_fileScanner;
    std::shared_ptr<repositories::HistoryRepository> m_historyRepo;
    std::shared_ptr<repositories::LibraryRepository> m_libraryRepo; // Added for persistence access in startScan
    
    // Hardware controller for S32K144 communication
    std::shared_ptr<controllers::HardwareController> m_hardwareController;
    
    // Views
    std::unique_ptr<views::NowPlayingBar> m_nowPlayingBar;
    std::unique_ptr<views::LibraryScreen> m_libraryScreen;
    std::unique_ptr<views::PlaylistScreen> m_playlistScreen;
    std::unique_ptr<views::QueuePanel> m_queuePanel;
    std::unique_ptr<views::HistoryScreen> m_historyScreen;
    
    // UI state
    int m_selectedIndex = 0;
    int m_scrollOffset = 0;
    
    std::atomic<bool> m_running;
    bool m_sdlInitialized;
};

} // namespace core
} // namespace media_player

#endif // APPLICATION_H