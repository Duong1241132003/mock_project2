#ifndef IMGUI_MANAGER_H
#define IMGUI_MANAGER_H

// SDL includes
#include <SDL2/SDL.h>
#include <SDL2/SDL_ttf.h>

// System includes
#include <string>
#include <functional>
#include <vector>
#include <memory>
#include <optional>
#include <map> // Added for view registration

#include "repositories/HistoryRepository.h"
#include "models/MediaFileModel.h"
#include "models/MetadataModel.h"

// Forward declarations
namespace media_player { 
namespace controllers { 
    class PlaybackController; 
    class QueueController;
    class LibraryController;
    class PlaylistController;
}

namespace views {
    class IView;
}

namespace ui 
{

// UI Theme colors
struct Theme {
    // Background colors
    uint32_t background      = 0xFF1E1E28;  // Dark background
    uint32_t surface         = 0xFF2A2A3A;  // Card/panel background  
    uint32_t surfaceHover    = 0xFF3A3A4A;  // Hover state
    uint32_t surfaceActive   = 0xFF4A4A5A;  // Active/selected
    
    // Primary accent
    uint32_t primary         = 0xFF6495ED;  // Cornflower blue
    uint32_t primaryHover    = 0xFF7AA5FF;
    uint32_t primaryActive   = 0xFF5585DD;
    
    // Text colors  
    uint32_t textPrimary     = 0xFFFFFFFF;
    uint32_t textSecondary   = 0xFFB4B4B4;
    uint32_t textDim         = 0xFF808080;
    
    // Status colors
    uint32_t success         = 0xFF32CD32;
    uint32_t warning         = 0xFFFFA500;
    uint32_t error           = 0xFFDC3232;
    
    // Utility
    uint32_t scrollbar       = 0xFF404050;
    uint32_t scrollbarThumb  = 0xFF606070;
    uint32_t border          = 0xFF404050; // Default dark theme border
    
    static Theme dark() { return Theme(); }
    static Theme light();
};

// Context menu source (where the right-click came from)
enum class ContextMenuSource { None, Library, Playlist, Queue };

// Search filter options
enum class SearchFilter {
    All,      // Search in title, artist, album
    Title,    // Search only in title
    Artist,   // Search only in artist
    Album     // Search only in album
};

// Navigation tabs
enum class NavTab {
    Library,
    Playlists,
    Queue,
    History,
    Settings
};

// UI State
struct UIState {
    NavTab currentTab = NavTab::Library;
    int selectedMediaIndex = -1;
    int scrollOffset = 0;
    int queueScrollOffset = 0;
    int historyScrollOffset = 0;
    float volume = 0.8f;
    bool isPlaying = false;
    bool isPlayingVideo = false;  // true khi đang phát video, dùng để hiển thị video panel
    SDL_Texture* videoTexture = nullptr;  // Texture video hiện tại để render
    int videoWidth = 0;
    int videoHeight = 0;
    std::string searchQuery;
    SearchFilter searchFilter = SearchFilter::All;  // Filter for search results
    std::string currentTrackTitle;
    std::string currentTrackArtist;
    float playbackProgress = 0.0f;
    float playbackDuration = 0.0f;
    
    // Sort options
    int sortField = 0;  // 0=Title, 1=Artist, 2=Duration
    bool sortAscending = true;
    
    // Pagination
    int currentPage = 0;
    static constexpr int ITEMS_PER_PAGE = 25;
    
    // Loop/Shuffle
    bool loopEnabled = false;   // true when LoopOne or LoopAll
    bool loopAllEnabled = false; // true when LoopAll (distinguishes from LoopOne)
    bool shuffleEnabled = false;
    
    // Playlist UI
    bool showCreatePlaylistDialog = false;
    std::string newPlaylistName;

    // Rename Playlist Dialog
    bool showRenamePlaylistDialog = false;
    std::string renamePlaylistId;
    std::string renamePlaylistName;

    std::string selectedPlaylistId;
    
    // Context Menu
    bool showContextMenu = false;
    int contextMenuX = 0;
    int contextMenuY = 0;
    int selectedContextItemIndex = -1; // Index in the current list (legacy, prefer contextMediaItem)
    models::MediaFileModel contextMediaItem; // The actual item to act upon
    ContextMenuSource contextMenuSource = ContextMenuSource::None;
    
    /** True when search box has focus (clicked) - enables text input for search. */
    bool searchFocused = false;
    
    // Add to Playlist Dialog
    bool showAddToPlaylistDialog = false;
    
    // Properties Dialog (read-only display)
    bool showPropertiesDialog = false;
    struct MetadataEditState {
        std::string title;
        std::string artist;
        std::string album;
        std::string genre;
        std::string year;
        std::string publisher;
        std::string durationStr;
        std::string bitrateStr;
        std::string filePath;
        std::string fileName;
        std::string extension;
        std::string typeStr;
        std::string fileSizeStr;
    } metadataEdit;
    
    // Library path (initial scan + change path)
    std::string libraryPathInput;
    std::string libraryPathError;
    bool focusPathInput = false;
    bool showChangePathDialog = false;
    /** True when the initial "enter path to scan" screen is visible (so text goes to path input, not search). */
    bool pathInputScreenVisible = false;
    /** True when scan progress dialog is visible (modal). */
    bool scanDialogVisible = false;

    // USB Popup
    bool showUsbDialog = false;
    std::string usbPath;
    
    // Hardware connection status (S32K144)
    bool hardwareConnected = false;
};

class ImGuiManager {
public:
    ImGuiManager();
    ~ImGuiManager();
    
    // Lifecycle
    bool initialize(const std::string& title, int width, int height);
    void shutdown();
    
    // Frame management
    void beginFrame();
    void endFrame();
    
    // Event handling
    bool processEvent(const SDL_Event& event);
    void handleResize(int width, int height);
    
    // Main UI rendering
    void renderMainLayout();
    
    // Components
    void renderMenuBar();
    void renderSidebar();
    void renderPlayerBar();
    void renderScanProgress(const std::string& path, int current, int total);
    void renderPathInputScreen(const std::string& currentPathPlaceholder);
    void renderOverlays();
    
    // Notifications
    void showUsbPopup(const std::string& path);
    
    // State access
    UIState& getState() { return m_state; }
    const UIState& getState() const { return m_state; }
    void setState(const UIState& state) { m_state = state; }
    
    // Getters
    SDL_Window* getWindow() const { return m_window; }
    SDL_Renderer* getRenderer() const { return m_renderer; }
    int getWidth() const { return m_width; }

    int getHeight() const { return m_height; }
    
    // Controllers (set by Application)
    void setControllers(
        std::shared_ptr<controllers::PlaybackController> playback,
        std::shared_ptr<controllers::QueueController> queue,
        std::shared_ptr<controllers::LibraryController> library,
        std::shared_ptr<controllers::PlaylistController> playlist
    );
    
    // Media list (set by Application)
    void setMediaList(const std::vector<models::MediaFileModel>* mediaList);
    
    // Callbacks
    using PlayCallback = std::function<void(int index)>;
    using VolumeCallback = std::function<void(float volume)>;
    using SeekCallback = std::function<void(float position)>;
    
    void setOnPlay(PlayCallback cb) { m_onPlay = std::move(cb); }
    void setOnVolumeChange(VolumeCallback cb) { m_onVolumeChange = std::move(cb); }
    void setOnSeek(SeekCallback cb) { m_onSeek = std::move(cb); }
    using RequestScanCallback = std::function<void(const std::string& path)>;
    using ChangeLibraryPathCallback = std::function<void(const std::string& path)>;
    using GetCurrentLibraryPathCallback = std::function<std::string()>;
    void setOnRequestScan(RequestScanCallback cb) { m_onRequestScan = std::move(cb); }
    void setOnChangeLibraryPath(ChangeLibraryPathCallback cb) { m_onChangeLibraryPath = std::move(cb); }
    void setGetCurrentLibraryPath(GetCurrentLibraryPathCallback cb) { m_getCurrentLibraryPath = std::move(cb); }
    using GetMetadataCallback = std::function<std::optional<models::MetadataModel>(const std::string& filePath)>;
    void setGetMetadataForProperties(GetMetadataCallback cb) { m_getMetadataForProperties = std::move(cb); }
    using QuitCallback = std::function<void()>;
    void setOnQuit(QuitCallback cb) { m_onQuit = std::move(cb); }
    using CancelScanCallback = std::function<void()>;
    void setOnCancelScan(CancelScanCallback cb) { m_onCancelScan = std::move(cb); }
    
    // View Registration
    void registerView(NavTab tab, views::IView* view);

public:
    // Rendering helpers (Make PUBLIC for Views to use)
    void drawRect(int x, int y, int w, int h, uint32_t color, bool filled = true);
    void drawRoundedRect(int x, int y, int w, int h, int radius, uint32_t color, bool filled = true);
    void drawText(const std::string& text, int x, int y, uint32_t color, int fontSize = 16);
    void drawIcon(const std::string& icon, int x, int y, uint32_t color, int size = 24);
    void drawProgressBar(int x, int y, int w, int h, float progress, uint32_t fg, uint32_t bg);
    void drawButton(const std::string& label, int x, int y, int w, int h, bool active = false);
    void drawSlider(float* value, int x, int y, int w, int h);
    
    // Hit testing (Make PUBLIC)
    bool isMouseOver(int x, int y, int w, int h) const {
        return m_mouseX >= x && m_mouseX < x + w && m_mouseY >= y && m_mouseY < y + h;
    }
    
    bool isMouseClicked(int x, int y, int w, int h) const {
        return m_mouseClicked && isMouseOver(x, y, w, h);
    }

    bool isLeftMouseClicked(int x, int y, int w, int h) const {
        return m_leftMouseClicked && isMouseOver(x, y, w, h);
    }

    bool isRightMouseClicked(int x, int y, int w, int h) const {
        return m_rightMouseClicked && isMouseOver(x, y, w, h);
    }
    
    bool isModalMouseClicked() const { return m_modalMouseClicked; }
    void consumeClick() { 
        m_mouseClicked = false; 
        m_leftMouseClicked = false;
        m_rightMouseClicked = false;
    }
    
    // Theme access
    const Theme& getTheme() const { return m_theme; }

private:
    bool m_mouseDragging = false;

    
    // SDL resources
    SDL_Window* m_window = nullptr;
    SDL_Renderer* m_renderer = nullptr;
    TTF_Font* m_font = nullptr;
    TTF_Font* m_fontLarge = nullptr;
    TTF_Font* m_fontSmall = nullptr;
    
    // Dimensions
    int m_width = 0;
    int m_height = 0;
    
    // Mouse state
    int m_mouseX = 0;
    int m_mouseY = 0;
    bool m_mouseDown = false;
    bool m_mouseClicked = false;
    bool m_leftMouseClicked = false;
    bool m_rightMouseClicked = false;
    bool m_modalMouseClicked = false; // True when modal dialog consumes click
    int m_dragStartX = 0;
    int m_dragStartY = 0;
    
    // UI State
    UIState m_state;
    Theme m_theme;
    
    // Data references
    const std::vector<models::MediaFileModel>* m_mediaList = nullptr;
    
    // Controllers
    std::shared_ptr<controllers::PlaybackController> m_playbackController;
    std::shared_ptr<controllers::QueueController> m_queueController;
    std::shared_ptr<controllers::LibraryController> m_libraryController;
    std::shared_ptr<controllers::PlaylistController> m_playlistController;
    
    // Callbacks
    PlayCallback m_onPlay;
    VolumeCallback m_onVolumeChange;
    SeekCallback m_onSeek;
    RequestScanCallback m_onRequestScan;
    ChangeLibraryPathCallback m_onChangeLibraryPath;
    GetCurrentLibraryPathCallback m_getCurrentLibraryPath;
    GetMetadataCallback m_getMetadataForProperties;
    QuitCallback m_onQuit;
    CancelScanCallback m_onCancelScan;
    
    // Layout constants
    static constexpr int SIDEBAR_WIDTH = 200;
    static constexpr int PLAYER_BAR_HEIGHT = 90;
    static constexpr int MENU_BAR_HEIGHT = 30;
    static constexpr int ITEM_HEIGHT = 50; // Keep protected/private if views don't need it or make public getter?
                                           // actually Views will need layout constants. Let's make them public static or just access via getters.

public:
    static constexpr int GetSidebarWidth() { return SIDEBAR_WIDTH; }
    static constexpr int GetPlayerBarHeight() { return PLAYER_BAR_HEIGHT; }
    static constexpr int GetMenuBarHeight() { return MENU_BAR_HEIGHT; }
    
private:
    std::map<NavTab, views::IView*> m_views;
};

} // namespace ui
} // namespace media_player

#endif // IMGUI_MANAGER_H
