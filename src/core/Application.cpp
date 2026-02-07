// Project includes
#include "core/Application.h"
#include "utils/Logger.h"
#include "config/AppConfig.h"
#include "ui/ImGuiManager.h"

// All models
#include "models/QueueModel.h"
#include "models/PlaybackStateModel.h"
#include "models/LibraryModel.h"
#include "models/SystemStateModel.h"

// All services
#include "services/FileScanner.h"
#include "services/MetadataReader.h"
#include "services/VideoMetadataReader.h"
#include "services/AudioPlaybackEngine.h"
#include "services/MetadataReader.h"
#include "services/VideoPlaybackEngine.h"
#include "services/SerialCommunication.h"

// All repositories
#include "repositories/LibraryRepository.h"
#include "repositories/PlaylistRepository.h"
#include "repositories/HistoryRepository.h"

// System
#include <thread>
#include <atomic>
#include <fstream>
#include <filesystem>

namespace media_player 
{
namespace core 
{

// Static UI manager instance
static std::unique_ptr<ui::ImGuiManager> g_uiManager;
static std::atomic<bool> g_scanComplete{false};
static std::atomic<bool> g_scanStarted{false};
static std::atomic<int> g_scanProgress{0};
static std::atomic<int> g_scanTotal{0};
static std::atomic<bool> g_scanCancelled{false};
static std::string g_currentScanPath;
static std::vector<models::MediaFileModel> g_scannedMedia;
static std::mutex g_mediaMutex;

static const char* LAST_SCAN_PATH_FILE = "./data/last_scan_path.txt";

static std::string loadLastScanPath() {
    std::ifstream f(LAST_SCAN_PATH_FILE);
    std::string path;
    if (f && std::getline(f, path)) {
        while (!path.empty() && (path.back() == '\r' || path.back() == '\n' || path.back() == ' ')) path.pop_back();
        if (!path.empty()) return path;
    }
    return config::AppConfig::DEFAULT_SCAN_PATH;
}

static void saveLastScanPath(const std::string& path) {
    std::filesystem::create_directories("./data");
    std::ofstream f(LAST_SCAN_PATH_FILE);
    if (f) f << path << "\n";
}

Application::Application()
    : m_running(false)
    , m_sdlInitialized(false)
{
    LOG_INFO("Application instance created");
}

Application::~Application() 
{
    g_uiManager.reset();
    
    if (m_sdlInitialized) 
    {
        SDL_Quit();
    }
    
    LOG_INFO("Application instance destroyed");
}

Application& Application::getInstance() 
{
    static Application instance;
    return instance;
}

bool Application::initialize() 
{
    LOG_INFO("Initializing application...");
    
    if (!initializeSDL()) 
    {
        LOG_ERROR("Failed to initialize SDL");
        return false;
    }
    
    // Initialize UI Manager with modern ImGui-style interface
    g_uiManager = std::make_unique<ui::ImGuiManager>();
    if (!g_uiManager->initialize("Media Player", 1280, 800)) 
    {
        LOG_ERROR("Failed to initialize UI Manager");
        return false;
    }
    
    if (!createAllComponents()) 
    {
        LOG_ERROR("Failed to create components");
        return false;
    }
    
    if (!wireUpDependencies()) 
    {
        LOG_ERROR("Failed to wire up dependencies");
        return false;
    }
    
    // Setup UI callbacks
    setupUICallbacks();
    
    // Start services/threads after callbacks are ready
    if (m_sourceController) {
        m_sourceController->startMonitoring();
    }
    
    LOG_INFO("Application initialized successfully");
    return true;
}

bool Application::initializeSDL() 
{
    if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_AUDIO | SDL_INIT_EVENTS) < 0) 
    {
        LOG_ERROR("SDL_Init failed: " + std::string(SDL_GetError()));
        return false;
    }
    
    m_sdlInitialized = true;
    LOG_INFO("SDL initialized");
    
    return true;
}

void Application::setupUICallbacks()
{
    if (!g_uiManager) return;
    
    // Set controllers
    g_uiManager->setControllers(m_playbackController, m_queueController, m_libraryController, m_playlistController);
    
    // Play callback - play without adding to queue (add via context menu when needed)
    g_uiManager->setOnPlay([this](int index) {
        std::lock_guard<std::mutex> lock(g_mediaMutex);
        if (m_libraryModel && index >= 0 && 
            index < static_cast<int>(m_libraryModel->getMediaCount())) 
        {
            auto mediaList = m_libraryModel->getAllMedia();
            const auto& media = mediaList[index];
            LOG_INFO("Playing: " + media.getFileName());
            
            if (m_playbackController && m_playbackController->playMediaWithoutQueue(media))
            {
                auto& state = g_uiManager->getState();
                state.isPlaying = true;
                std::string title = media.getTitle();
                state.currentTrackTitle = title.empty() ? media.getFileName() : title;
                std::string artist = media.getArtist();
                state.currentTrackArtist = artist.empty() ? "Unknown Artist" : artist;
                state.playbackDuration = static_cast<float>(media.getDuration());
            }
        }
    });
    
    // Volume change callback
    g_uiManager->setOnVolumeChange([this](float volume) {
        if (m_playbackController) {
            // Convert 0.0-1.0 to 0-100
            m_playbackController->setVolume(static_cast<int>(volume * 100));
        }
    });
    
    // Callback for state changes (Thread-safe polling handles the rest)
    m_playbackStateModel->setStateChangeCallback([this](models::PlaybackState state) {
        // Optional: Could trigger an event to wake up main loop if sleeping
    });

    // Metadata changes are polled in update(), no need for callback here which runs on audio thread
    m_playbackStateModel->setMetadataChangeCallback(nullptr);
    
    // Request scan (path input screen): validate path; if invalid use last path as fallback
    g_uiManager->setOnRequestScan([this](const std::string& path) {
        std::string toScan = path;
        while (!toScan.empty() && (toScan.back() == ' ' || toScan.back() == '\n')) toScan.pop_back();
        bool valid = !toScan.empty() && std::filesystem::exists(toScan) && std::filesystem::is_directory(toScan);
        if (valid) {
            if (g_uiManager) g_uiManager->getState().libraryPathError.clear();
            // Use resetAndRescan to clear old library before loading new source
            resetAndRescan(toScan);
        } else {
            std::string fallback = loadLastScanPath();
            if (g_uiManager)
                g_uiManager->getState().libraryPathError = "Đường dẫn không hợp lệ. Đang dùng: " + fallback;
            // Fallback also needs reset if we were trying to switch
            resetAndRescan(fallback);
        }
    });
    
    g_uiManager->setGetCurrentLibraryPath([]() { return loadLastScanPath(); });
    
    g_uiManager->setGetMetadataForProperties([this](const std::string& path) {
        return m_libraryController ? m_libraryController->readMetadata(path) : std::optional<models::MetadataModel>();
    });
    g_uiManager->setOnQuit([this]() { quit(); });
    
    // Cancel scan callback
    g_uiManager->setOnCancelScan([this]() {
        LOG_INFO("User cancelled scan");
        g_scanCancelled = true;
        if (m_fileScanner && m_fileScanner->isScanning())
        {
            m_fileScanner->stopScanning();
        }
        g_scanComplete = true;
        g_scanStarted = false;
    });
    
    // Change library path (from Settings): reset playback, queue, library, history then rescan
    g_uiManager->setOnChangeLibraryPath([this](const std::string& path) {
        std::string toScan = path;
        while (!toScan.empty() && (toScan.back() == ' ' || toScan.back() == '\n')) toScan.pop_back();
        if (toScan.empty() || !std::filesystem::exists(toScan) || !std::filesystem::is_directory(toScan)) {
            if (g_uiManager) g_uiManager->getState().libraryPathError = "Đường dẫn không hợp lệ.";
            return;
        }
        if (g_uiManager) {
            g_uiManager->getState().showChangePathDialog = false;
            g_uiManager->getState().libraryPathError.clear();
        }
        resetAndRescan(toScan);
    });
    
    // USB Inserted Callback
    m_sourceController->setUsbInsertedCallback([this](const std::string& path) {
        // Push event to main thread
        SDL_Event event;
        SDL_zero(event);
        event.type = EVENT_USB_INSERTED;
        // Allocate string on heap to pass through event data
        event.user.data1 = new std::string(path); 
        SDL_PushEvent(&event);
    });
    
    // Setup Hardware Controller Callbacks (S32K144)
    if (m_hardwareController)
    {
        // Volume callback: ADC message "!ADC: %d!" điều chỉnh volume 0-100
        m_hardwareController->setVolumeCallback([this](int volume) {
            if (m_playbackController)
            {
                m_playbackController->setVolume(volume);
                LOG_INFO("Hardware volume set to: " + std::to_string(volume));
                
                // Cập nhật UI state
                if (g_uiManager)
                {
                    g_uiManager->getState().volume = static_cast<float>(volume) / 100.0f;
                }
            }
        });
        
        // Button callback: BTN message "!BTN: %d!" điều khiển playback
        m_hardwareController->setButtonCallback([this](controllers::HardwareButton button) {
            if (!m_playbackController)
            {
                return;
            }
            
            switch (button)
            {
                case controllers::HardwareButton::TOGGLE_PLAY_PAUSE:
                    // BTN 1: Toggle Play/Pause
                    m_playbackController->togglePlayPause();
                    LOG_INFO("Hardware: Toggle Play/Pause");
                    break;
                    
                case controllers::HardwareButton::NEXT:
                    // BTN 2: Next track
                    m_playbackController->playNext();
                    LOG_INFO("Hardware: Next track");
                    break;
                    
                case controllers::HardwareButton::PREVIOUS:
                    // BTN 3: Previous track
                    m_playbackController->playPrevious();
                    LOG_INFO("Hardware: Previous track");
                    break;
                    
                case controllers::HardwareButton::STOP:
                    // BTN 4: Stop
                    m_playbackController->stop();
                    LOG_INFO("Hardware: Stop");
                    break;
            }
        });
    }
}

bool Application::createAllComponents() 
{
    LOG_INFO("Creating application components...");
    
    // Create models
    auto queueModel = std::make_shared<models::QueueModel>();
    auto playbackStateModel = std::make_shared<models::PlaybackStateModel>();
    auto libraryModel = std::make_shared<models::LibraryModel>();
    auto systemStateModel = std::make_shared<models::SystemStateModel>();
    
    // Create repositories
    auto libraryRepo = std::make_shared<repositories::LibraryRepository>(
        config::AppConfig::LIBRARY_STORAGE_PATH
    );
    auto playlistRepo = std::make_shared<repositories::PlaylistRepository>(
        config::AppConfig::PLAYLIST_STORAGE_PATH
    );
    m_historyRepo = std::make_shared<repositories::HistoryRepository>(
        config::AppConfig::HISTORY_STORAGE_PATH
    );
    
    // Create services
    auto fileScanner = std::make_shared<services::FileScanner>();
    auto serialComm = std::make_shared<services::SerialCommunication>();
    auto metadataReader = std::make_shared<services::MetadataReader>();
    
    // Create controllers
    m_queueController = std::make_shared<controllers::QueueController>(queueModel);
    
    m_playbackController = std::make_shared<controllers::PlaybackController>(
        queueModel,
        playbackStateModel,
        m_historyRepo
    );
    
    m_videoController = std::make_shared<controllers::VideoController>(
        m_playbackController,
        playbackStateModel
    );
    
    m_sourceController = std::make_shared<controllers::SourceController>(
        fileScanner,
        libraryRepo,
        libraryModel
    );
    
    m_libraryController = std::make_shared<controllers::LibraryController>(
        libraryModel,
        libraryRepo,
        metadataReader
    );
    
    m_playlistController = std::make_shared<controllers::PlaylistController>(
        playlistRepo
    );
    
    m_hardwareController = std::make_shared<controllers::HardwareController>(
        serialComm,
        playbackStateModel
    );
    
    m_mainController = std::make_shared<controllers::MainController>(
        m_playbackController,
        m_videoController,
        m_sourceController,
        m_libraryController,
        m_playlistController,
        m_queueController,
        m_hardwareController,
        systemStateModel
    );
    
    // Store models for UI access
    m_libraryModel = libraryModel;
    m_playbackStateModel = playbackStateModel;
    m_fileScanner = fileScanner;
    
    // Create views (kept for compatibility, but UI now uses ImGuiManager)
    m_nowPlayingBar = std::make_unique<views::NowPlayingBar>(
        playbackStateModel,
        m_playbackController
    );
    
    m_libraryScreen = std::make_unique<views::LibraryScreen>(
        m_libraryController,
        m_queueController
    );
    
    m_playlistScreen = std::make_unique<views::PlaylistScreen>(
        m_playlistController
    );
    
    m_queuePanel = std::make_unique<views::QueuePanel>(
        m_queueController,
        queueModel
    );
    
    m_videoPlayerScreen = std::make_unique<views::VideoPlayerScreen>(
        m_videoController,
        m_playbackController,
        playbackStateModel
    );
    
    LOG_INFO("All components created successfully");
    return true;
}

bool Application::wireUpDependencies() 
{
    LOG_INFO("Wiring up dependencies...");
    
    // Initialize main controller
    if (!m_mainController->initialize()) 
    {
        LOG_ERROR("Failed to initialize main controller");
        return false;
    }
    
    // Truyền renderer từ main UI vào VideoPlaybackEngine để video render trong main UI
    if (g_uiManager && m_playbackController)
    {
        SDL_Renderer* renderer = g_uiManager->getRenderer();
        if (renderer)
        {
            m_playbackController->setExternalRenderer(renderer);
            LOG_INFO("External renderer set for video playback");
        }
    }
    
    LOG_INFO("Dependencies wired successfully");
    return true;
}

void Application::startScan(const std::string& path)
{
    g_scanComplete = false;
    g_scanStarted = true;
    g_scanProgress = 0;
    g_scanTotal = 0;
    g_currentScanPath = path;
    g_scanCancelled = false;
    
    std::thread scanThread([this, path]() {
        LOG_INFO("Starting media scan: " + path);
        if (m_fileScanner)
        {
            m_fileScanner->setProgressCallback([](int current, int total, const std::string& p) {
                g_scanProgress = current;
                g_scanTotal = total;
                g_currentScanPath = p;
            });
            auto scannedMedia = m_fileScanner->scanDirectorySync(path);
            {
                std::lock_guard<std::mutex> lock(g_mediaMutex);
                g_scannedMedia = scannedMedia;
            }

            if (!g_scanCancelled)
            {
                // Chỉ cập nhật thư viện khi scan hoàn tất thành công
                if (m_libraryModel)
                {
                    m_libraryModel->clear();
                    m_libraryModel->addMediaBatch(g_scannedMedia);
                }
            }
            else
            {
                LOG_INFO("Scan cancelled, keeping existing library");
            }
        }
        g_scanComplete = true;
        LOG_INFO("Media scan complete. Found " + std::to_string(g_scannedMedia.size()) + " files");
    });
    scanThread.detach();
}

void Application::resetAndRescan(const std::string& path)
{
    if (m_playbackController) m_playbackController->stop();
    if (m_queueController) m_queueController->clearQueue();
    if (m_historyRepo) m_historyRepo->clear();
    saveLastScanPath(path);
    
    // Reset UI state to prevent "ghost" pages or invalid selection
    if (g_uiManager) {
        auto& state = g_uiManager->getState();
        state.currentPage = 0;
        state.selectedMediaIndex = -1;
        state.scrollOffset = 0;
        state.searchQuery.clear();
        state.searchFocused = false;
        // Keep sort settings or reset? Resetting is safer.
        state.sortField = 0;
        state.sortAscending = true;
    }
    
    startScan(path);
}

int Application::run() 
{
    LOG_INFO("Starting main application loop...");
    
    m_running = true;
    
    // Auto-start scan on launch to skip the "Select Source" screen
    std::string lastPath = loadLastScanPath();
    if (lastPath.empty() || !std::filesystem::exists(lastPath)) {
        const char* homeDir = getenv("HOME");
        if (homeDir) {
            std::string musicDir = std::string(homeDir) + "/Music";
            if (std::filesystem::exists(musicDir)) lastPath = musicDir;
            else lastPath = std::string(homeDir);
        } else {
            lastPath = "/";
        }
    }
    
    // Start scanning immediately
    if (g_uiManager) {
        g_uiManager->getState().pathInputScreenVisible = false;
        g_uiManager->getState().focusPathInput = false;
        SDL_StopTextInput(); 
    }
    startScan(lastPath);
    
    // Main loop
    while (m_running) 
    {
        processEvents();
        update();
        render();
        
        // Cap frame rate to ~60 FPS
        SDL_Delay(16);
    }
    
    LOG_INFO("Main application loop ended");
    return 0;
}

void Application::quit() 
{
    LOG_INFO("Application quit requested");
    m_running = false;
}

void Application::processEvents() 
{
    SDL_Event event;

    if (g_uiManager)
    {
        g_uiManager->getState().scanDialogVisible = (!g_scanComplete && g_scanStarted);
    }
    
    while (SDL_PollEvent(&event)) 
    {
        // Let ImGuiManager process events first
        if (g_uiManager) {
            g_uiManager->processEvent(event);
        }
        
        switch (event.type) 
        {
            case SDL_QUIT:
                quit();
                break;
                
            case SDL_KEYDOWN:
                handleKeyboardEvent(event.key);
                break;
                
            case SDL_WINDOWEVENT:
                handleWindowEvent(event.window);
                break;
                
            default:
                if (event.type == EVENT_PLAYBACK_FINISHED)
                {
                    if (m_playbackController)
                    {
                        m_playbackController->onFinished();
                    }
                }
                else if (event.type == EVENT_VIDEO_FRAME_READY)
                {
                    // Update texture từ main thread (SDL operations phải từ main thread)
                    if (m_playbackController)
                    {
                        m_playbackController->updateTextureFromMainThread();
                    }
                }
                else if (event.type == EVENT_USB_INSERTED)
                {
                    // Extract path from user data
                    std::string* pathPtr = static_cast<std::string*>(event.user.data1);
                    if (pathPtr && g_uiManager)
                    {
                        g_uiManager->showUsbPopup(*pathPtr);
                        delete pathPtr; // Free heap memory
                    }
                }
                break;
        }
    }
}

void Application::update() 
{
    // Path input screen visibility (so UI can route keyboard to search vs path input)
    if (g_uiManager) {
        g_uiManager->getState().pathInputScreenVisible = (!g_scanComplete && !g_scanStarted);
        if (g_uiManager->getState().pathInputScreenVisible) {
            auto& state = g_uiManager->getState();
            if (state.libraryPathInput.empty())
                state.libraryPathInput = loadLastScanPath();
        }
    }
    
    // Update UI state with playback info
    if (g_uiManager && m_playbackStateModel) {
        auto& state = g_uiManager->getState();
        state.isPlaying = (m_playbackStateModel->getState() == models::PlaybackState::PLAYING);
        
        // Update metadata and progress
        state.currentTrackTitle = m_playbackStateModel->getCurrentTitle();
        state.currentTrackArtist = m_playbackStateModel->getCurrentArtist();

        float duration = static_cast<float>(m_playbackStateModel->getTotalDuration());
        state.playbackDuration = duration;
        
        float current = static_cast<float>(m_playbackStateModel->getCurrentPosition());
        if (duration > 0) {
            state.playbackProgress = current / duration;
        } else {
            state.playbackProgress = 0.0f;
        }
        
        // Also sync Loop/Shuffle state from Queue Controller if available
        if (m_queueController) {
             state.loopEnabled = m_queueController->isRepeatEnabled();
             state.loopAllEnabled = m_queueController->isLoopAllEnabled();
             state.shuffleEnabled = m_queueController->isShuffleEnabled();
        }
        
        // Cập nhật state video cho UI
        if (m_playbackController)
        {
            state.isPlayingVideo = m_playbackController->isPlayingVideo();
            
            if (state.isPlayingVideo)
            {
                // Lấy video texture và resolution
                state.videoTexture = m_playbackController->getCurrentVideoTexture();
                m_playbackController->getVideoResolution(state.videoWidth, state.videoHeight);
            }
            else
            {
                state.videoTexture = nullptr;
                state.videoWidth = 0;
                state.videoHeight = 0;
            }
        }
    }
    
    // Update media list reference
    if (g_uiManager && m_libraryModel && g_scanComplete) {
        std::lock_guard<std::mutex> lock(g_mediaMutex);
        static std::vector<models::MediaFileModel> cachedMedia;
        cachedMedia = m_libraryModel->getAllMedia();
        g_uiManager->setMediaList(&cachedMedia);
    }
    
    // Sync playback history to UI
    if (g_uiManager && m_historyRepo) {
        auto historyList = m_historyRepo->getRecentHistory(100);
        g_uiManager->setHistoryList(historyList);
    }
    
    // Thử kết nối lại phần cứng S32K144 theo chu kỳ để nhận bản tin từ UART
    if (m_hardwareController)
    {
        m_hardwareController->refreshConnection();
    }

    // Cập nhật trạng thái kết nối phần cứng S32K144
    if (g_uiManager && m_hardwareController)
    {
        g_uiManager->getState().hardwareConnected = m_hardwareController->isConnected();
    }
}

void Application::render() 
{
    if (!g_uiManager) return;
    
    g_uiManager->beginFrame();
    
    if (!g_scanComplete && g_scanStarted) 
    {
        g_uiManager->getState().scanDialogVisible = true;
        g_uiManager->renderMainLayout();
        g_uiManager->renderScanProgress(g_currentScanPath, g_scanProgress, g_scanTotal);
    }
    else if (!g_scanComplete && !g_scanStarted) 
    {
        g_uiManager->renderMainLayout();
        g_uiManager->renderPathInputScreen(loadLastScanPath());
    }
    else 
    {
        g_uiManager->renderMainLayout();
    }
    
    g_uiManager->endFrame();
}

void Application::handleKeyboardEvent(const SDL_KeyboardEvent& event) 
{
    SDL_Keycode key = event.keysym.sym;
    
    // Check for modal dialogs blocking input
    if (g_uiManager && g_uiManager->getState().showUsbDialog) {
        // Allow Quit (Ctrl+Q) only? Or block everything? 
        // User said "require interaction with this screen", so strict blocking.
        // But we might want to allow Esc to close it if that's UI policy.
        // For now, adhere to request: only interact with the popup.
        // Allowing ONLY Quit for safety.
        if (key == SDLK_q && (event.keysym.mod & KMOD_CTRL)) 
        {
             quit();
        }
        return;
    }
    
    // Global shortcuts
    if (key == SDLK_q || (key == SDLK_q && (event.keysym.mod & KMOD_CTRL))) 
    {
        quit();
        return;
    }
    
    if (!g_uiManager) return;
    auto& state = g_uiManager->getState();
    
    // Only handle keys in Library view for now
    if (state.currentTab == ui::NavTab::Library && g_scanComplete && m_libraryModel) 
    {
        int mediaCount = static_cast<int>(m_libraryModel->getMediaCount());
        
        if (key == SDLK_UP || key == SDLK_k) 
        {
            if (state.selectedMediaIndex > 0) 
            {
                state.selectedMediaIndex--;
                // Scroll up if needed
                if (state.selectedMediaIndex < state.scrollOffset) 
                {
                    state.scrollOffset = state.selectedMediaIndex;
                }
            }
        }
        else if (key == SDLK_DOWN || key == SDLK_j) 
        {
            if (state.selectedMediaIndex < mediaCount - 1) 
            {
                state.selectedMediaIndex++;
                // Scroll down if needed
                int visibleItems = (g_uiManager->getHeight() - 180) / 50;
                if (state.selectedMediaIndex >= state.scrollOffset + visibleItems) 
                {
                    state.scrollOffset = state.selectedMediaIndex - visibleItems + 1;
                }
            }
        }
        else if (key == SDLK_RETURN || key == SDLK_KP_ENTER) 
        {
            if (state.selectedMediaIndex >= 0 && state.selectedMediaIndex < mediaCount) 
            {
                auto mediaList = m_libraryModel->getAllMedia();
                const auto& media = mediaList[state.selectedMediaIndex];
                LOG_INFO("Playing: " + media.getFileName());
                
                if (m_playbackController && m_playbackController->playMediaWithoutQueue(media))
                {
                    state.isPlaying = true;
                    state.currentTrackTitle = media.getTitle().empty() ? media.getFileName() : media.getTitle();
                    state.currentTrackArtist = media.getArtist().empty() ? "Unknown Artist" : media.getArtist();
                }
            }
        }
        else if (key == SDLK_SPACE) 
        {
            // Toggle play/pause
            if (m_playbackController) 
            {
                m_playbackController->togglePlayPause();
                state.isPlaying = !state.isPlaying;
            }
        }
        else if (key == SDLK_PAGEUP) 
        {
            int visibleItems = (g_uiManager->getHeight() - 180) / 50;
            state.selectedMediaIndex = std::max(0, state.selectedMediaIndex - visibleItems);
            state.scrollOffset = std::max(0, state.scrollOffset - visibleItems);
        }
        else if (key == SDLK_PAGEDOWN) 
        {
            int visibleItems = (g_uiManager->getHeight() - 180) / 50;
            state.selectedMediaIndex = std::min(mediaCount - 1, state.selectedMediaIndex + visibleItems);
            if (state.selectedMediaIndex >= state.scrollOffset + visibleItems) 
            {
                state.scrollOffset = state.selectedMediaIndex - visibleItems + 1;
            }
        }
        else if (key == SDLK_HOME) 
        {
            state.selectedMediaIndex = 0;
            state.scrollOffset = 0;
        }
        else if (key == SDLK_END) 
        {
            state.selectedMediaIndex = mediaCount - 1;
            int visibleItems = (g_uiManager->getHeight() - 180) / 50;
            state.scrollOffset = std::max(0, mediaCount - visibleItems);
        }
    }
    
    // Tab navigation - only when no dialog is active
    if (!state.showCreatePlaylistDialog && !state.showRenamePlaylistDialog && 
        !state.showAddToPlaylistDialog && !state.showPropertiesDialog &&
        !state.showChangePathDialog && !state.pathInputScreenVisible) {
        if (key == SDLK_1) state.currentTab = ui::NavTab::Library;
        else if (key == SDLK_2) state.currentTab = ui::NavTab::Playlists;
        else if (key == SDLK_3) state.currentTab = ui::NavTab::Queue;
        else if (key == SDLK_4) state.currentTab = ui::NavTab::History;
    }
    
    // Pass to video player if visible
    if (m_videoPlayerScreen && m_videoPlayerScreen->isVisible()) 
    {
        m_videoPlayerScreen->handleKeyPress(key);
    }
}

void Application::handleMouseEvent(const SDL_MouseButtonEvent& event) 
{
    // Mouse events are now handled by ImGuiManager
    (void)event;
}

void Application::handleWindowEvent(const SDL_WindowEvent& event) 
{
    if (event.event == SDL_WINDOWEVENT_RESIZED) 
    {
        // Handle window resize
        if (m_videoPlayerScreen && m_videoPlayerScreen->isVisible()) 
        {
            m_videoPlayerScreen->handleWindowResize(event.data1, event.data2);
        }
        
        if (g_uiManager) {
            g_uiManager->handleResize(event.data1, event.data2);
        }
    }
}

} // namespace core
} // namespace media_player