// Project includes
#include "ui/ImGuiManager.h"
#include "utils/Logger.h"
#include "models/MediaFileModel.h"
#include "controllers/PlaybackController.h"
#include "controllers/QueueController.h"
#include "controllers/LibraryController.h"
#include "controllers/PlaylistController.h"
#include "models/PlaylistModel.h"

// SDL includes
#include <SDL2/SDL_ttf.h>

// System includes
#include <sstream>
#include <iomanip>
#include <algorithm>
#include <cmath>
#include <fstream>
#include <chrono>
#include <filesystem>

namespace media_player 
{
namespace ui 
{

// Helper to extract RGBA from packed color
static void unpackColor(uint32_t color, Uint8& r, Uint8& g, Uint8& b, Uint8& a) {
    a = (color >> 24) & 0xFF;
    r = (color >> 16) & 0xFF;
    g = (color >> 8) & 0xFF;
    b = color & 0xFF;
}

Theme Theme::light() {
    Theme t;
    // Modern Light Theme
    t.background      = 0xFFF5F6FA;  // Very soft gray/white background (Lynx White)
    t.surface         = 0xFFFFFFFF;  // Pure white cards/panels
    t.surfaceHover    = 0xFFE8EAF6;  // Very light indigo tint for hover
    t.surfaceActive   = 0xFFDCE1E8;  // Light gray-blue active
    
    // Primary - use a vibrant but professional blue (similar to iOS/macOS blue)
    t.primary         = 0xFF007AFF;  
    t.primaryHover    = 0xFF3395FF;
    t.primaryActive   = 0xFF0056B3;
    
    // Text - High contrast dark gray (not full black for softness)
    t.textPrimary     = 0xFF2C3E50;  // Dark Blue-Gray (Midnight Blue)
    t.textSecondary   = 0xFF7F8C8D;  // Medium Gray (Asbestos)
    t.textDim         = 0xFF95A5A6;  // Light Gray (Concrete)
    
    // Status
    t.success         = 0xFF2ECC71;  // Emerald Green
    t.warning         = 0xFFF1C40F;  // Sun Flower Yellow
    t.error           = 0xFFE74C3C;  // Alizarin Red
    
    // UI Elements
    t.border          = 0xFFDCDCDC;  // Light gray border
    t.scrollbar       = 0xFFF0F0F0;
    t.scrollbarThumb  = 0xFFBDC3C7;
    
    return t;
}

ImGuiManager::ImGuiManager() {
    // Set default theme to Light
    m_theme = Theme::light();
}

ImGuiManager::~ImGuiManager() {
    shutdown();
}

bool ImGuiManager::initialize(const std::string& title, int width, int height) {
    m_width = width;
    m_height = height;
    
    // Initialize SDL_ttf
    if (TTF_Init() < 0) {
        LOG_ERROR("Failed to initialize SDL_ttf: " + std::string(TTF_GetError()));
        return false;
    }
    
    // Create window with OpenGL context for better rendering
    m_window = SDL_CreateWindow(
        title.c_str(),
        SDL_WINDOWPOS_CENTERED,
        SDL_WINDOWPOS_CENTERED,
        width,
        height,
        SDL_WINDOW_SHOWN | SDL_WINDOW_RESIZABLE | SDL_WINDOW_ALLOW_HIGHDPI
    );
    
    if (!m_window) {
        LOG_ERROR("Failed to create window: " + std::string(SDL_GetError()));
        return false;
    }
    
    // Create hardware-accelerated renderer
    m_renderer = SDL_CreateRenderer(
        m_window,
        -1,
        SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC
    );
    
    if (!m_renderer) {
        LOG_ERROR("Failed to create renderer: " + std::string(SDL_GetError()));
        return false;
    }
    
    // Enable alpha blending
    SDL_SetRenderDrawBlendMode(m_renderer, SDL_BLENDMODE_BLEND);
    
    // Load fonts
    const char* fontPaths[] = {
        "/usr/share/fonts/truetype/dejavu/DejaVuSans.ttf",
        "/usr/share/fonts/truetype/freefont/FreeSans.ttf",
        "/usr/share/fonts/TTF/DejaVuSans.ttf",
        "/usr/share/fonts/liberation/LiberationSans-Regular.ttf",
        "/usr/share/fonts/noto/NotoSans-Regular.ttf"
    };
    
    for (const char* path : fontPaths) {
        m_font = TTF_OpenFont(path, 14);
        if (m_font) {
            m_fontLarge = TTF_OpenFont(path, 20);
            m_fontSmall = TTF_OpenFont(path, 11);
            LOG_INFO("Loaded font: " + std::string(path));
            break;
        }
    }
    
    if (!m_font) {
        LOG_WARNING("No system font found, text rendering will be limited");
    }
    
    LOG_INFO("ImGuiManager initialized: " + std::to_string(width) + "x" + std::to_string(height));
    
    // Disable text input by default, enable only on focus
    SDL_StopTextInput();
    
    return true;
}

void ImGuiManager::shutdown() {
    if (m_fontSmall) {
        TTF_CloseFont(m_fontSmall);
        m_fontSmall = nullptr;
    }
    if (m_fontLarge) {
        TTF_CloseFont(m_fontLarge);
        m_fontLarge = nullptr;
    }
    if (m_font) {
        TTF_CloseFont(m_font);
        m_font = nullptr;
    }
    if (m_renderer) {
        SDL_DestroyRenderer(m_renderer);
        m_renderer = nullptr;
    }
    if (m_window) {
        SDL_DestroyWindow(m_window);
        m_window = nullptr;
    }
    TTF_Quit();
}

void ImGuiManager::beginFrame() {
    // Clear with background color
    Uint8 r, g, b, a;
    unpackColor(m_theme.background, r, g, b, a);
    SDL_SetRenderDrawColor(m_renderer, r, g, b, a);
    SDL_RenderClear(m_renderer);
    
    // Get current window size
    SDL_GetWindowSize(m_window, &m_width, &m_height);

    // Reset modal click state per frame - MOVED TO ENDFRAME
    // m_modalMouseClicked = false;

    m_state.scanDialogVisible = false;
}

void ImGuiManager::endFrame() {
    SDL_RenderPresent(m_renderer);
    
    // Reset click state after rendering so all UI elements can check it
    m_mouseClicked = false;
    m_modalMouseClicked = false;
}

bool ImGuiManager::processEvent(const SDL_Event& event) {
    switch (event.type) {
        case SDL_MOUSEMOTION:
            m_mouseX = event.motion.x;
            m_mouseY = event.motion.y;
            return true;
            
        case SDL_MOUSEBUTTONDOWN:
            if (event.button.button == SDL_BUTTON_LEFT)
            {
                m_mouseDown = true;
                
                // Modal check for USB Dialog
                if (m_state.showUsbDialog) 
                {
                    m_modalMouseClicked = true;
                    // Do NOT set m_mouseClicked = true here.
                    // Only interaction with the specific dialog code in renderOverlays 
                    // should be allowed, which can use m_modalMouseClicked if needed, 
                    // or we just trust m_mouseClicked which will be false?
                    // Wait, renderOverlays uses m_mouseClicked.
                    // If we want to block others, we should set a specialized flag or 
                    // ensure others check "if (!showUsbDialog)" which is tedious.
                    // Better approach: Set m_mouseClicked = true, but ensure other panels check modality.
                    // OR: Use m_modalMouseClicked for modals and reset m_mouseClicked to false?
                    
                    // Let's rely on the strategy:
                    // If modal, set m_modalMouseClicked = true.
                    // Set m_mouseClicked = FALSE (so standard panels ignore it).
                    // Update renderOverlays to use m_modalMouseClicked for the USB dialog.
                    m_mouseClicked = false; 
                }
                else if (m_state.showChangePathDialog)
                {
                    m_modalMouseClicked = true;
                    m_mouseClicked = false;
                }
                else if (m_state.scanDialogVisible)
                {
                    m_modalMouseClicked = true;
                    m_mouseClicked = false;
                }
                else
                {
                    m_mouseClicked = true;
                    m_modalMouseClicked = false;
                }
                m_dragStartX = event.button.x;
                m_dragStartY = event.button.y;
            }
            return true;
            
        case SDL_MOUSEBUTTONUP:
            if (event.button.button == SDL_BUTTON_LEFT) {
                m_mouseDown = false;
            }
            return true;
            
        case SDL_MOUSEWHEEL:
            if (m_state.currentTab == NavTab::Library) {
                m_state.scrollOffset -= event.wheel.y * 30;
                if (m_state.scrollOffset < 0) m_state.scrollOffset = 0;
            } else if (m_state.currentTab == NavTab::Queue) {
                m_state.queueScrollOffset -= event.wheel.y * 30;
                if (m_state.queueScrollOffset < 0) m_state.queueScrollOffset = 0;
            } else if (m_state.currentTab == NavTab::History) {
                m_state.historyScrollOffset -= event.wheel.y * 30;
                if (m_state.historyScrollOffset < 0) m_state.historyScrollOffset = 0;
            }
            return true;
        
        case SDL_TEXTINPUT:
            if (m_state.focusPathInput && (m_state.pathInputScreenVisible || m_state.showChangePathDialog)) {
                m_state.libraryPathInput += event.text.text;
            } else if (m_state.showCreatePlaylistDialog) {
                m_state.newPlaylistName += event.text.text;
            } else if (m_state.showRenamePlaylistDialog) {
                m_state.renamePlaylistName += event.text.text;
            } else if (m_state.currentTab == NavTab::Library) {
                // Add text to search query
                m_state.searchQuery += event.text.text;
                m_state.currentPage = 0;
                m_state.scrollOffset = 0;
            }
            return true;
            
        case SDL_KEYDOWN:
            if (!m_state.pathInputScreenVisible && !m_state.showChangePathDialog)
                m_state.focusPathInput = false;
            if (m_state.focusPathInput && (m_state.pathInputScreenVisible || m_state.showChangePathDialog)) {
                if (event.key.keysym.sym == SDLK_BACKSPACE && !m_state.libraryPathInput.empty()) {
                    m_state.libraryPathInput.pop_back();
                } else if (event.key.keysym.sym == SDLK_RETURN || event.key.keysym.sym == SDLK_KP_ENTER) {
                    if (m_state.showChangePathDialog && m_onChangeLibraryPath)
                        m_onChangeLibraryPath(m_state.libraryPathInput);
                    else if (m_onRequestScan)
                        m_onRequestScan(m_state.libraryPathInput);
                } else if (event.key.keysym.sym == SDLK_ESCAPE) {
                    m_state.focusPathInput = false;
                    if (m_state.showChangePathDialog) m_state.showChangePathDialog = false;
                    SDL_StopTextInput();
                }
            } else if (m_state.showCreatePlaylistDialog) {
                // Playlist Dialog Input
                if (event.key.keysym.sym == SDLK_BACKSPACE && !m_state.newPlaylistName.empty()) {
                    m_state.newPlaylistName.pop_back();
                } else if (event.key.keysym.sym == SDLK_ESCAPE) {
                    m_state.showCreatePlaylistDialog = false;
                    m_state.newPlaylistName.clear();
                    SDL_StopTextInput();
                } else if (event.key.keysym.sym == SDLK_RETURN || event.key.keysym.sym == SDLK_KP_ENTER) {
                    if (!m_state.newPlaylistName.empty() && m_playlistController) {
                        m_playlistController->createPlaylist(m_state.newPlaylistName);
                        m_state.showCreatePlaylistDialog = false;
                        m_state.newPlaylistName.clear();
                        SDL_StopTextInput();
                    }
                }
            } else if (m_state.showRenamePlaylistDialog) {
                if (event.key.keysym.sym == SDLK_BACKSPACE && !m_state.renamePlaylistName.empty()) {
                    m_state.renamePlaylistName.pop_back();
                } else if (event.key.keysym.sym == SDLK_ESCAPE) {
                    m_state.showRenamePlaylistDialog = false;
                    SDL_StopTextInput();
                } else if (event.key.keysym.sym == SDLK_RETURN || event.key.keysym.sym == SDLK_KP_ENTER) {
                    if (!m_state.renamePlaylistName.empty() && m_playlistController) {
                        m_playlistController->renamePlaylist(m_state.renamePlaylistId, m_state.renamePlaylistName);
                    }
                    m_state.showRenamePlaylistDialog = false;
                    SDL_StopTextInput();
                }
            } else {
                // Global Shortcuts / Search
                if (event.key.keysym.sym == SDLK_BACKSPACE && !m_state.searchQuery.empty()) {
                    m_state.searchQuery.pop_back();
                    m_state.currentPage = 0;
                    m_state.scrollOffset = 0;
                } else if (event.key.keysym.sym == SDLK_ESCAPE) {
                    m_state.searchQuery.clear();
                    m_state.searchFocused = false;
                    m_state.currentPage = 0;
                    m_state.scrollOffset = 0;
                }
            }
            return true;
            
        default:
            break;
    }
    return false;
}

void ImGuiManager::handleResize(int width, int height) {
    m_width = width;
    m_height = height;
}

void ImGuiManager::renderMainLayout() {
    renderMenuBar();
    renderSidebar();
    renderPlayerBar();
    
    // Main content area
    int contentX = SIDEBAR_WIDTH;
    int contentY = MENU_BAR_HEIGHT;
    int contentW = m_width - SIDEBAR_WIDTH;
    int contentH = m_height - MENU_BAR_HEIGHT - PLAYER_BAR_HEIGHT;
    
    // Draw content background
    drawRect(contentX, contentY, contentW, contentH, m_theme.background);
    
    // Nếu đang phát video, render video panel thay vì content thông thường
    if (m_state.isPlayingVideo && m_state.videoTexture)
    {
        // Tính toán kích thước video giữ tỷ lệ khung hình
        int videoW = m_state.videoWidth;
        int videoH = m_state.videoHeight;
        
        if (videoW > 0 && videoH > 0)
        {
            float aspectRatio = static_cast<float>(videoW) / static_cast<float>(videoH);
            float contentAspect = static_cast<float>(contentW) / static_cast<float>(contentH);
            
            int renderW, renderH;
            if (aspectRatio > contentAspect)
            {
                // Video rộng hơn - fit theo chiều ngang
                renderW = contentW;
                renderH = static_cast<int>(contentW / aspectRatio);
            }
            else
            {
                // Video cao hơn - fit theo chiều dọc
                renderH = contentH;
                renderW = static_cast<int>(contentH * aspectRatio);
            }
            
            // Center video trong content area
            int renderX = contentX + (contentW - renderW) / 2;
            int renderY = contentY + (contentH - renderH) / 2;
            
            // Render video texture
            SDL_Rect destRect = {renderX, renderY, renderW, renderH};
            SDL_RenderCopy(m_renderer, m_state.videoTexture, nullptr, &destRect);
        }
        
        // Vẫn render overlays (context menu, dialogs, etc.)
        renderOverlays();
        return;
    }
    
    // Render current view
    switch (m_state.currentTab) {
        case NavTab::Library:
            renderLibraryPanel();
            break;
        case NavTab::Playlists:
            renderPlaylistPanel();
            break;
        case NavTab::Queue:
            renderQueuePanel();
            break;
        case NavTab::History:
            renderHistoryPanel();
            break;
        case NavTab::Settings: {
            drawText("Settings", contentX + 20, contentY + 20, m_theme.textPrimary, 24);
            int sy = contentY + 60;
            drawText("Thư mục thư viện:", contentX + 20, sy, m_theme.textSecondary, 14);
            std::string currentPath = m_getCurrentLibraryPath ? m_getCurrentLibraryPath() : "";
            if (currentPath.length() > 50) currentPath = "..." + currentPath.substr(currentPath.length() - 47);
            drawText(currentPath, contentX + 20, sy + 22, m_theme.textPrimary, 12);
            int btnX = contentX + 20;
            int btnY = sy + 55;
            int btnW = 140;
            int btnH = 32;
            bool changeHover = isMouseOver(btnX, btnY, btnW, btnH);
            drawRect(btnX, btnY, btnW, btnH, changeHover ? m_theme.primaryHover : m_theme.primary);
            drawText("Đổi đường dẫn", btnX + 18, btnY + 8, m_theme.textPrimary, 14);
            if (changeHover && m_mouseClicked) {
                m_state.libraryPathInput = m_getCurrentLibraryPath ? m_getCurrentLibraryPath() : "";
                m_state.showChangePathDialog = true;
                m_state.focusPathInput = true;
                SDL_StartTextInput();
                m_mouseClicked = false;
            }
            break;
        }
    }
    
    renderOverlays();
}

void ImGuiManager::renderMenuBar() {
    // Menu bar background
    drawRect(0, 0, m_width, MENU_BAR_HEIGHT, m_theme.surface);
    
    // App title
    drawText("Media Player", 10, 6, m_theme.textPrimary, 14);
    
    // Search box - only show when Library tab is active
    if (m_state.currentTab == NavTab::Library) {
        int searchX = m_width - 250;
        int searchW = 200;
        int searchY = 4;
        int searchH = MENU_BAR_HEIGHT - 8;
        bool searchHover = isMouseOver(searchX, searchY, searchW, searchH);
        drawRect(searchX, searchY, searchW, searchH, (m_state.searchFocused || searchHover) ? m_theme.surfaceActive : m_theme.surfaceHover, true);
        if (searchHover && m_mouseClicked) {
            m_state.searchFocused = true;
            SDL_Rect searchRect = {searchX, searchY, searchW, searchH};
            SDL_SetTextInputRect(&searchRect);
            SDL_StartTextInput();
        }
        if (m_state.searchQuery.empty()) {
            drawText("Search...", searchX + 8, 7, m_theme.textDim, 12);
        } else {
            std::string displayQuery = m_state.searchQuery;
            if (displayQuery.length() > 25) displayQuery = displayQuery.substr(0, 22) + "...";
            drawText(displayQuery, searchX + 8, 7, m_theme.textPrimary, 12);
        }
        if (m_state.searchFocused && m_mouseClicked && !searchHover) {
            m_state.searchFocused = false;
        }
    } else {
        m_state.searchFocused = false;
    }
    
    // Bottom border
    drawRect(0, MENU_BAR_HEIGHT - 1, m_width, 1, m_theme.border);
}

void ImGuiManager::renderSidebar() {
    int y = MENU_BAR_HEIGHT;
    int h = m_height - MENU_BAR_HEIGHT - PLAYER_BAR_HEIGHT;
    
    // Sidebar background
    drawRect(0, y, SIDEBAR_WIDTH, h, m_theme.surface);
    
    // Navigation items
    struct NavItem {
        const char* icon;
        const char* label;
        NavTab tab;
    };
    
    NavItem items[] = {
        {">", "Library", NavTab::Library},
        {"#", "Playlists", NavTab::Playlists},
        {"=", "Queue", NavTab::Queue},
        {"@", "History", NavTab::History}
    };
    
    int itemY = y + 20;
    int itemH = 40;
    
    for (const auto& item : items) {
        bool selected = (m_state.currentTab == item.tab);
        bool hover = isMouseOver(0, itemY, SIDEBAR_WIDTH, itemH);
        
        // Background
        if (selected) {
            drawRect(0, itemY, SIDEBAR_WIDTH, itemH, m_theme.primary);
        } else if (hover) {
            drawRect(0, itemY, SIDEBAR_WIDTH, itemH, m_theme.surfaceHover);
        }
        
        // Accent bar for selected
        if (selected) {
            drawRect(0, itemY, 4, itemH, m_theme.primaryActive);
        }
        
        // Icon and label
        uint32_t textColor = selected ? m_theme.textPrimary : 
                            (hover ? m_theme.textPrimary : m_theme.textSecondary);
        
        drawText(item.icon, 15, itemY + 10, textColor, 16);
        drawText(item.label, 45, itemY + 12, textColor, 14);
        
        // Handle click
        if (hover && m_mouseClicked) {
            m_state.currentTab = item.tab;
            m_state.searchFocused = false;
        }
        
        itemY += itemH;
    }
    
    // Separator line
    drawRect(10, itemY + 10, SIDEBAR_WIDTH - 20, 1, m_theme.border);
    itemY += 25;
    
    // "Change Source" button - below History
    bool changeSourceHover = isMouseOver(10, itemY, SIDEBAR_WIDTH - 20, 36);
    uint32_t changeBtnColor = changeSourceHover ? m_theme.surfaceHover : m_theme.surface;
    drawRect(10, itemY, SIDEBAR_WIDTH - 20, 36, changeBtnColor);
    drawRect(10, itemY, SIDEBAR_WIDTH - 20, 36, m_theme.border, false);
    
    uint32_t changeTextColor = changeSourceHover ? m_theme.textPrimary : m_theme.textSecondary;
    drawText("~", 20, itemY + 8, changeTextColor, 16);
    drawText("Change Source", 45, itemY + 10, changeTextColor, 12);
    
    // Stop (Quit) Button - Bottom Left
    int stopBtnH = 36;
    int stopBtnY = m_height - PLAYER_BAR_HEIGHT - stopBtnH - 10;
    bool stopHover = isMouseOver(10, stopBtnY, SIDEBAR_WIDTH - 20, stopBtnH);
    drawRect(10, stopBtnY, SIDEBAR_WIDTH - 20, stopBtnH, stopHover ? m_theme.error : m_theme.surfaceActive);
    drawText("Quit App", 45, stopBtnY + 10, m_theme.textPrimary, 12);
    
    if (stopHover && m_mouseClicked && m_onQuit) {
        m_onQuit();
        m_mouseClicked = false;
    }
    
    if (changeSourceHover && m_mouseClicked) {
        // Open the change path dialog
        m_state.libraryPathInput = m_getCurrentLibraryPath ? m_getCurrentLibraryPath() : "";
        m_state.showChangePathDialog = true;
        m_state.focusPathInput = true;
        SDL_StartTextInput();
        m_mouseClicked = false;
    }
    
    // Right border
    drawRect(SIDEBAR_WIDTH - 1, y, 1, h, m_theme.border);
}

void ImGuiManager::renderLibraryPanel() {
    if (!m_mediaList) return;
    
    int x = SIDEBAR_WIDTH + 10;
    int y = MENU_BAR_HEIGHT + 10;
    int w = m_width - SIDEBAR_WIDTH - 20;
    int h = m_height - MENU_BAR_HEIGHT - PLAYER_BAR_HEIGHT - 20;
    
    // 1. Calculate filtered list first
    std::vector<size_t> filteredIndices;
    filteredIndices.reserve(m_mediaList->size());
    
    for (size_t i = 0; i < m_mediaList->size(); i++) {
        const auto& media = (*m_mediaList)[i];
        
        if (m_state.searchQuery.empty()) {
            filteredIndices.push_back(i);
        } else {
            // Case-insensitive search
            std::string query = m_state.searchQuery;
            std::transform(query.begin(), query.end(), query.begin(), ::tolower);
            
            std::string title = media.getTitle().empty() ? media.getFileName() : media.getTitle();
            std::transform(title.begin(), title.end(), title.begin(), ::tolower);
            
            std::string artist = media.getArtist();
            std::transform(artist.begin(), artist.end(), artist.begin(), ::tolower);
            
            std::string album = media.getAlbum();
            std::transform(album.begin(), album.end(), album.begin(), ::tolower);
            
            bool matches = false;
            switch (m_state.searchFilter) {
                case SearchFilter::All:
                    matches = (title.find(query) != std::string::npos || 
                               artist.find(query) != std::string::npos ||
                               album.find(query) != std::string::npos);
                    break;
                case SearchFilter::Title:
                    matches = (title.find(query) != std::string::npos);
                    break;
                case SearchFilter::Artist:
                    matches = (artist.find(query) != std::string::npos);
                    break;
                case SearchFilter::Album:
                    matches = (album.find(query) != std::string::npos);
                    break;
            }
            if (matches) {
                filteredIndices.push_back(i);
            }
        }
    }
    
    int filteredCount = static_cast<int>(filteredIndices.size());
    int totalPages = (filteredCount + UIState::ITEMS_PER_PAGE - 1) / UIState::ITEMS_PER_PAGE;
    if (totalPages == 0) totalPages = 1;

    // Sort Logic including Album (Field 3? Let's use 2 and move Duration to 3)
    // 0=Title, 1=Artist, 2=Album, 3=Duration
    if (m_state.sortField != -1 && !filteredIndices.empty()) { 
         std::stable_sort(filteredIndices.begin(), filteredIndices.end(), 
            [&](size_t a, size_t b) {
                const auto& mA = (*m_mediaList)[a];
                const auto& mB = (*m_mediaList)[b];
                int cmp = 0;
                if (m_state.sortField == 0) { // Title
                    std::string tA = mA.getTitle().empty() ? mA.getFileName() : mA.getTitle();
                    std::string tB = mB.getTitle().empty() ? mB.getFileName() : mB.getTitle();
                    cmp = tA.compare(tB);
                } else if (m_state.sortField == 1) { // Artist
                    cmp = mA.getArtist().compare(mB.getArtist());
                } else if (m_state.sortField == 2) { // Album
                    cmp = mA.getAlbum().compare(mB.getAlbum());
                } else if (m_state.sortField == 3) { // Duration
                    int dA = mA.getDuration();
                    int dB = mB.getDuration();
                    cmp = (dA < dB) ? -1 : ((dA > dB) ? 1 : 0);
                }
                return m_state.sortAscending ? (cmp < 0) : (cmp > 0);
            });
    }

    // 2. Header
    drawText("Library", x, y, m_theme.textPrimary, 20);
    
    // Search filter buttons (only show when search query is not empty)
    if (!m_state.searchQuery.empty()) {
        int filterX = x + 90;
        int filterY = y + 2;
        int filterBtnW = 55;
        int filterBtnH = 22;
        int filterGap = 5;
        
        struct FilterBtn {
            const char* label;
            SearchFilter filter;
        };
        FilterBtn filters[] = {
            {"All", SearchFilter::All},
            {"Title", SearchFilter::Title},
            {"Artist", SearchFilter::Artist},
            {"Album", SearchFilter::Album}
        };
        
        for (const auto& fb : filters) {
            bool selected = (m_state.searchFilter == fb.filter);
            bool hover = isMouseOver(filterX, filterY, filterBtnW, filterBtnH);
            
            uint32_t btnColor = selected ? m_theme.primary : 
                               (hover ? m_theme.surfaceHover : m_theme.surface);
            drawRect(filterX, filterY, filterBtnW, filterBtnH, btnColor);
            drawRect(filterX, filterY, filterBtnW, filterBtnH, m_theme.border, false);
            
            uint32_t textColor = selected ? m_theme.textPrimary : m_theme.textSecondary;
            drawText(fb.label, filterX + 8, filterY + 4, textColor, 11);
            
            if (hover && m_mouseClicked) {
                m_state.searchFilter = fb.filter;
                m_state.currentPage = 0;
                m_mouseClicked = false;
            }
            
            filterX += filterBtnW + filterGap;
        }
        
        // Clear Search Button (X) - After filters
        int clearBtnSize = 22;
        int clearX = filterX + 5;
        
        bool clearHover = isMouseOver(clearX, filterY, clearBtnSize, clearBtnSize);
        if (clearHover) {
             drawRect(clearX, filterY, clearBtnSize, clearBtnSize, m_theme.surfaceActive);
        }
        drawRect(clearX, filterY, clearBtnSize, clearBtnSize, m_theme.border, false);
        drawText("x", clearX + 7, filterY + 2, m_theme.textDim, 14);
        
        if (clearHover && m_mouseClicked) {
            m_state.searchQuery.clear();
            m_state.currentPage = 0;
            m_state.scrollOffset = 0;
            SDL_StopTextInput();
            m_mouseClicked = false;
        }
    }
    
    std::string pageInfo = "Page " + std::to_string(m_state.currentPage + 1) + "/" + std::to_string(totalPages);
    std::string countText = std::to_string(filteredCount) + " tracks";
    
    drawText(countText, x + w - 180, y + 4, m_theme.textDim, 12);
    drawText(pageInfo, x + w - 80, y + 4, m_theme.textSecondary, 12);
    
    y += 40;
    
    // 3. Column headers
    int colTitle = x;
    int colArtist = x + w * 0.35;
    int colAlbum = x + w * 0.6;
    int colDuration = x + w - 70;
    
    drawRect(x, y, w, 30, m_theme.surface);
    
    // Title header
    bool titleHover = isMouseOver(colTitle, y, static_cast<int>(w * 0.35), 30);
    std::string titleHeader = "Title";
    if (m_state.sortField == 0) titleHeader += m_state.sortAscending ? " ^" : " v";
    drawText(titleHeader, colTitle + 50, y + 7, titleHover ? m_theme.textPrimary : m_theme.textSecondary, 12);
    if (titleHover && m_mouseClicked) {
        if (m_state.sortField == 0) m_state.sortAscending = !m_state.sortAscending;
        else { m_state.sortField = 0; m_state.sortAscending = true; }
    }
    
    // Artist header
    bool artistHover = isMouseOver(colArtist, y, static_cast<int>(w * 0.25), 30);
    std::string artistHeader = "Artist";
    if (m_state.sortField == 1) artistHeader += m_state.sortAscending ? " ^" : " v";
    drawText(artistHeader, colArtist, y + 7, artistHover ? m_theme.textPrimary : m_theme.textSecondary, 12);
    if (artistHover && m_mouseClicked) {
        if (m_state.sortField == 1) m_state.sortAscending = !m_state.sortAscending;
        else { m_state.sortField = 1; m_state.sortAscending = true; }
    }

    // Album header
    bool albumHover = isMouseOver(colAlbum, y, static_cast<int>(w * 0.25), 30);
    std::string albumHeader = "Album";
    if (m_state.sortField == 2) albumHeader += m_state.sortAscending ? " ^" : " v";
    drawText(albumHeader, colAlbum, y + 7, albumHover ? m_theme.textPrimary : m_theme.textSecondary, 12);
    if (albumHover && m_mouseClicked) {
        if (m_state.sortField == 2) m_state.sortAscending = !m_state.sortAscending;
        else { m_state.sortField = 2; m_state.sortAscending = true; }
    }
    
    // Duration header
    bool durHover = isMouseOver(colDuration, y, 70, 30);
    std::string durHeader = "Time";
    if (m_state.sortField == 3) durHeader += m_state.sortAscending ? " ^" : " v";
    drawText(durHeader, colDuration, y + 7, durHover ? m_theme.textPrimary : m_theme.textSecondary, 12);
    if (durHover && m_mouseClicked) {
        if (m_state.sortField == 3) m_state.sortAscending = !m_state.sortAscending;
        else { m_state.sortField = 3; m_state.sortAscending = true; }
    }
    
    y += 30;
    
    // 4. Media list
    int listH = h - 110; // Leave room for pagination buttons (40px buttons + padding)
    
    // Set Clip Rect (if we had SDL_RenderSetClipRect)
    SDL_Rect clipRect = { x, y, w, listH };
    SDL_RenderSetClipRect(m_renderer, &clipRect);

    // int visibleItems = listH / ITEM_HEIGHT; // Unused since using pagination loop
    
    // Pagination logic
    if (m_state.currentPage >= totalPages) m_state.currentPage = totalPages - 1;
    if (m_state.currentPage < 0) m_state.currentPage = 0;
    
    int startIndex = m_state.currentPage * UIState::ITEMS_PER_PAGE;
    int endIndex = std::min(startIndex + UIState::ITEMS_PER_PAGE, filteredCount);
    
    // Use visibleItems? No, we use Pagination now (25 items/page).
    // Render the current page items
    
    for (int i = startIndex; i < endIndex; i++) {
        size_t index = filteredIndices[i];
        const auto& media = (*m_mediaList)[index];
        
        // Render item relative to y + Scroll Offset
        int rowIdx = i - startIndex; // 0 to 24
        int itemY = y + (rowIdx * ITEM_HEIGHT) - m_state.scrollOffset;
        
        // Culling (skip items outside view)
        if (itemY + ITEM_HEIGHT < y || itemY > y + listH) continue;
        
        bool selected = (index == static_cast<size_t>(m_state.selectedMediaIndex));
        bool hover = isMouseOver(x, itemY, w, ITEM_HEIGHT); // Note: isMouseOver needs to account for Clip Rect if implemented in engine, but here simple coord check
        // Check if mouse inside List Area
        bool mouseInList = isMouseOver(x, y, w, listH);
        hover = hover && mouseInList;

        uint32_t rowBg = selected ? m_theme.surfaceActive : 
                        (hover ? m_theme.surfaceHover : 
                        (i % 2 == 0 ? m_theme.background : m_theme.surface));
        drawRect(x, itemY, w, ITEM_HEIGHT, rowBg);
        
        // Right click context menu
        int mx, my;
        uint32_t buttons = SDL_GetMouseState(&mx, &my);
        if (hover && mouseInList && (buttons & SDL_BUTTON(SDL_BUTTON_RIGHT))) {
             m_state.showContextMenu = true;
             m_state.contextMenuX = mx;
             m_state.contextMenuY = my;
             m_state.selectedContextItemIndex = static_cast<int>(index);
             m_state.contextMediaItem = (*m_mediaList)[index];
             m_state.contextMenuSource = ui::ContextMenuSource::Library;
        }
        
        if (index == static_cast<size_t>(m_state.selectedMediaIndex) && m_state.isPlaying) {
            drawRect(x, itemY, 4, ITEM_HEIGHT, m_theme.success);
            drawText(">", x + 15, itemY + 15, m_theme.success, 14);
        } else {
            const char* icon = media.isAudio() ? "~" : "*";
            drawText(icon, x + 15, itemY + 15, m_theme.textSecondary, 14);
        }
        
        std::string title = media.getTitle().empty() ? media.getFileName() : media.getTitle();
        if (title.length() > 35) title = title.substr(0, 32) + "...";
        drawText(title, colTitle + 50, itemY + 15, m_theme.textPrimary, 14);
        
        std::string artist = media.getArtist().empty() ? "Unknown Artist" : media.getArtist();
        if (artist.length() > 25) artist = artist.substr(0, 22) + "...";
        drawText(artist, colArtist, itemY + 15, m_theme.textSecondary, 14);
        
        std::string album = media.getAlbum();
        if (album.length() > 25) album = album.substr(0, 22) + "...";
        drawText(album, colAlbum, itemY + 15, m_theme.textSecondary, 14);

        int duration = media.getDuration();
        std::string durationStr = "--:--";
        if (duration > 0) {
            int mins = duration / 60;
            int secs = duration % 60;
            std::ostringstream oss;
            oss << mins << ":" << std::setfill('0') << std::setw(2) << secs;
            durationStr = oss.str();
        }
        drawText(durationStr, colDuration, itemY + 15, m_theme.textDim, 12);
        
        if (hover && m_mouseClicked && !m_state.showContextMenu && !m_state.showAddToPlaylistDialog && !m_state.showPropertiesDialog && !m_state.showCreatePlaylistDialog && !m_state.showRenamePlaylistDialog) {
            m_state.selectedMediaIndex = static_cast<int>(index);
            if (m_onPlay) {
                m_onPlay(static_cast<int>(index));
                m_mouseClicked = false; // Consume click to prevent play/pause toggle
            }
        }
    }
    
    // Reset Clip Rect
    SDL_RenderSetClipRect(m_renderer, nullptr);

    // 5. Pagination Buttons (Fixed at bottom of list area)
    int pageBtnsY = y + listH + 10;
    int prevBtnX = x + w/2 - 100;
    int nextBtnX = x + w/2 + 20;

    if (m_state.currentPage > 0) {
        bool prevHover = isMouseOver(prevBtnX, pageBtnsY, 80, 28);
        drawRect(prevBtnX, pageBtnsY, 80, 28, prevHover ? m_theme.primaryHover : m_theme.primary);
        drawText("< Prev", prevBtnX + 15, pageBtnsY + 6, m_theme.textPrimary, 12);
        if (prevHover && m_mouseClicked) m_state.currentPage--;
    }

    if (m_state.currentPage < totalPages - 1) {
        bool nextHover = isMouseOver(nextBtnX, pageBtnsY, 80, 28);
        drawRect(nextBtnX, pageBtnsY, 80, 28, nextHover ? m_theme.primaryHover : m_theme.primary);
        drawText("Next >", nextBtnX + 15, pageBtnsY + 6, m_theme.textPrimary, 12);
        if (nextHover && m_mouseClicked) m_state.currentPage++;
    }
}

void ImGuiManager::renderPlayerBar() {
    int y = m_height - PLAYER_BAR_HEIGHT;
    
    // Background
    drawRect(0, y, m_width, PLAYER_BAR_HEIGHT, m_theme.surface);
    
    // Top border
    drawRect(0, y, m_width, 1, m_theme.border);
    
    // Album art placeholder
    int artSize = 70;
    int artX = 10;
    int artY = y + 10;
    drawRect(artX, artY, artSize, artSize, m_theme.surfaceHover);
    drawText("~", artX + 25, artY + 22, m_theme.textDim, 24);
    
    // Track info (truncate so text does not overlap playback controls)
    int infoX = artX + artSize + 15;
    int maxTitleLen = 36;
    int maxArtistLen = 36;
    std::string title = m_state.currentTrackTitle.empty() ? "No track playing" : m_state.currentTrackTitle;
    if (title.length() > static_cast<size_t>(maxTitleLen)) title = title.substr(0, maxTitleLen - 3) + "...";
    std::string artist = m_state.currentTrackArtist.empty() ? "" : m_state.currentTrackArtist;
    if (artist.length() > static_cast<size_t>(maxArtistLen)) artist = artist.substr(0, maxArtistLen - 3) + "...";
    
    drawText(title, infoX, y + 25, m_theme.textPrimary, 16);
    if (!artist.empty()) {
        drawText(artist, infoX, y + 48, m_theme.textSecondary, 12);
    }
    
    // Playback controls (center)
    int controlsX = m_width / 2 - 70;
    int controlsY = y + 15;
    
    // Previous button
    bool prevHover = isMouseOver(controlsX, controlsY, 35, 35);
    drawRect(controlsX, controlsY, 35, 35, prevHover ? m_theme.surfaceHover : m_theme.surface);
    drawText("|<", controlsX + 8, controlsY + 8, prevHover ? m_theme.textPrimary : m_theme.textSecondary, 16);
    if (prevHover && m_mouseClicked && m_playbackController) {
        m_playbackController->playPrevious();
    }
    
    // Play/Pause button
    int playX = controlsX + 45;
    bool playHover = isMouseOver(playX, controlsY, 40, 40);
    drawRect(playX, controlsY, 40, 40, playHover ? m_theme.primaryHover : m_theme.primary, true);
    const char* playIcon = m_state.isPlaying ? "||" : ">";
    drawText(playIcon, playX + 12, controlsY + 8, m_theme.textPrimary, 20);
    
    if (playHover && m_mouseClicked && m_playbackController) {
        if (m_state.isPlaying) {
            m_playbackController->pause();
            m_state.isPlaying = false;
        } else {
            m_playbackController->play();
            m_state.isPlaying = true;
        }
    }
    
    // Next button  
    int nextX = controlsX + 100;
    bool nextHover = isMouseOver(nextX, controlsY, 35, 35);
    drawRect(nextX, controlsY, 35, 35, nextHover ? m_theme.surfaceHover : m_theme.surface);
    drawText(">|", nextX + 8, controlsY + 8, nextHover ? m_theme.textPrimary : m_theme.textSecondary, 16);
    if (nextHover && m_mouseClicked && m_playbackController) {
        m_playbackController->playNext();
    }
    
    // Progress bar
    int progressX = m_width / 2 - 150;
    int progressY = y + 55;
    int progressW = 300;
    int progressH = 6;
    
    // Time labels
    std::string currentTime = "0:00";
    std::string totalTime = "0:00";
    
    if (m_state.playbackDuration > 0) {
        int currentSec = static_cast<int>(m_state.playbackProgress * m_state.playbackDuration);
        int totalSec = static_cast<int>(m_state.playbackDuration);
        
        std::ostringstream currentSS, totalSS;
        currentSS << (currentSec / 60) << ":" << std::setfill('0') << std::setw(2) << (currentSec % 60);
        totalSS << (totalSec / 60) << ":" << std::setfill('0') << std::setw(2) << (totalSec % 60);
        currentTime = currentSS.str();
        totalTime = totalSS.str();
    }
    
    drawText(currentTime, progressX - 45, progressY - 3, m_theme.textDim, 11);
    drawProgressBar(progressX, progressY, progressW, progressH, m_state.playbackProgress, 
                    m_theme.primary, m_theme.scrollbar);
    drawText(totalTime, progressX + progressW + 10, progressY - 3, m_theme.textDim, 11);
    
    // Handle progress bar click for seeking
    if (isMouseOver(progressX, progressY - 5, progressW, progressH + 10) && m_mouseClicked) {
        float seekPos = static_cast<float>(m_mouseX - progressX) / progressW;
        seekPos = std::clamp(seekPos, 0.0f, 1.0f);
        m_state.playbackProgress = seekPos;
        if (m_playbackController && m_state.playbackDuration > 0) {
            int seekSeconds = static_cast<int>(seekPos * m_state.playbackDuration);
            m_playbackController->seek(seekSeconds);
        }
    }
    
    // Volume control (right side)
    int volumeX = m_width - 180;
    int volumeY = y + 35;
    
    drawText("Vol:", volumeX - 5, volumeY, m_theme.textSecondary, 12);
    
    int sliderX = volumeX + 30;
    int sliderW = 100;
    int sliderH = 6;
    
    drawRect(sliderX, volumeY + 5, sliderW, sliderH, m_theme.scrollbar);
    int volumeW = static_cast<int>(sliderW * m_state.volume);
    drawRect(sliderX, volumeY + 5, volumeW, sliderH, m_theme.primary);
    
    // Volume knob
    int knobX = sliderX + volumeW - 4;
    drawRect(knobX, volumeY + 2, 8, 12, m_theme.textPrimary, true);
    
    // Handle volume drag
    if (isMouseOver(sliderX, volumeY, sliderW, 20) && m_mouseDown) {
        float newVolume = static_cast<float>(m_mouseX - sliderX) / sliderW;
        m_state.volume = std::clamp(newVolume, 0.0f, 1.0f);
        if (m_onVolumeChange) {
            m_onVolumeChange(m_state.volume);
        }
    }
    
    // Loop / Shuffle buttons (larger hit area so they register reliably)
    int modeX = volumeX - 80;
    const int modeBtnW = 28;
    const int modeBtnH = 28;
    
    // Shuffle
    bool shuffleHover = isMouseOver(modeX, volumeY - 2, modeBtnW, modeBtnH);
    uint32_t shuffleColor = m_state.shuffleEnabled ? m_theme.primary : (shuffleHover ? m_theme.textPrimary : m_theme.textDim);
    drawText("S", modeX + 8, volumeY + 2, shuffleColor, 14);
    if (shuffleHover && m_mouseClicked && m_queueController) {
        m_queueController->toggleShuffle();
        m_state.shuffleEnabled = m_queueController->isShuffleEnabled();
        m_mouseClicked = false;
    }
    
    // Loop (cycles: None -> LoopOne -> LoopAll -> None)
    int loopX = modeX + 34;
    bool loopHover = isMouseOver(loopX, volumeY - 2, modeBtnW, modeBtnH);
    uint32_t loopColor = m_state.loopEnabled ? m_theme.primary : (loopHover ? m_theme.textPrimary : m_theme.textDim);
    const char* loopLabel = m_state.loopAllEnabled ? "L+" : "L";
    drawText(loopLabel, loopX + (m_state.loopAllEnabled ? 4 : 8), volumeY + 2, loopColor, 14);
    if (loopHover && m_mouseClicked && m_queueController) {
        m_queueController->cycleRepeatMode();
        m_state.loopEnabled = m_queueController->isRepeatEnabled();
        m_state.loopAllEnabled = m_queueController->isLoopAllEnabled();
        m_mouseClicked = false;
    }
    
    // Hardware connection status indicator removed as per user request
}

void ImGuiManager::renderQueuePanel() {
    int x = SIDEBAR_WIDTH + 10;
    int y = MENU_BAR_HEIGHT + 10;
    int w = m_width - SIDEBAR_WIDTH - 20;
    
    // Header
    std::string headerText = "Queue";
    if (m_queueController) {
        headerText += " (" + std::to_string(m_queueController->getQueueSize()) + " items)";
    }
    drawText(headerText, x, y, m_theme.textPrimary, 20);
    
    // Clear Queue Button (if not empty)
    if (m_queueController && !m_queueController->isEmpty()) {
        int clearBtnW = 100;
        if (isMouseOver(x + w - clearBtnW, y, clearBtnW, 25)) {
            drawRect(x + w - clearBtnW, y, clearBtnW, 25, m_theme.surfaceHover);
            if (m_mouseClicked) {
                m_queueController->clearQueue();
            }
        } else {
            drawRect(x + w - clearBtnW, y, clearBtnW, 25, m_theme.surface);
        }
        drawText("Clear Queue", x + w - clearBtnW + 10, y + 5, m_theme.textDim, 12);
    }

    y += 40;
    
    // Now Playing section
    drawText("Now Playing", x, y, m_theme.textSecondary, 14);
    y += 25;
    
    drawRect(x, y, w, 60, m_theme.surface);
    if (!m_state.currentTrackTitle.empty()) {
        int npArtSize = 40;
        drawRect(x + 10, y + 10, npArtSize, npArtSize, m_theme.surfaceHover);
        drawText("~", x + 23, y + 20, m_theme.textDim, 20); // Placeholder art
        
        drawText(m_state.currentTrackTitle, x + 60, y + 12, m_theme.primary, 16);
        drawText(m_state.currentTrackArtist, x + 60, y + 35, m_theme.textSecondary, 12);
    } else {
        drawText("No track playing", x + 20, y + 20, m_theme.textDim, 14);
    }
    y += 80;
    
    // Up Next section
    drawText("Up Next", x, y, m_theme.textSecondary, 14);
    y += 25;
    
    if (!m_queueController || m_queueController->isEmpty()) {
        // Queue is empty message
        drawRect(x, y, w, 100, m_theme.surface);
        drawText("Queue is empty", x + 20, y + 40, m_theme.textDim, 14);
        drawText("Play a song from Library to start", x + 20, y + 60, m_theme.textDim, 12);
    } else {
        // Queue List (playback order when shuffle on)
        auto queueItems = m_queueController->getPlaybackOrderItems();
        size_t currentLogicalIndex = m_queueController->getCurrentIndex();
        int listH = m_height - y - PLAYER_BAR_HEIGHT - 20;
        
        // Items to display
        // Calculate visible area for scrolling
        
        // Clip rect for list
        SDL_Rect clipRect = { x, y, w, listH };
        SDL_RenderSetClipRect(m_renderer, &clipRect);
        
        // Scrollable content height
        // int totalContentH = queueItems.size() * ITEM_HEIGHT;
        
        for (size_t i = 0; i < queueItems.size(); i++) {
            const auto& media = queueItems[i];
            
            // Calculate Y with scroll offset
            int itemY = y + (i * ITEM_HEIGHT) - m_state.queueScrollOffset;
            
            // Culling
            if (itemY + ITEM_HEIGHT < y || itemY > y + listH) continue;
            
            bool hover = isMouseOver(x, itemY, w, ITEM_HEIGHT);
            // Check if mouse inside List Area
            bool mouseInList = isMouseOver(x, y, w, listH);
            hover = hover && mouseInList;
            
            drawRect(x, itemY, w, ITEM_HEIGHT, hover ? m_theme.surfaceHover : (i % 2 == 0 ? m_theme.background : m_theme.surface));
            
            // Context Menu (Right Click)
            int mx, my;
            uint32_t buttons = SDL_GetMouseState(&mx, &my);
            if (hover && mouseInList && (buttons & SDL_BUTTON(SDL_BUTTON_RIGHT))) {
                 m_state.showContextMenu = true;
                 m_state.contextMenuX = mx;
                 m_state.contextMenuY = my;
                 m_state.contextMediaItem = media;
                 m_state.contextMenuSource = ui::ContextMenuSource::Queue;
            }
            
            // Index (position in playback order)
            bool isCurrent = (m_state.isPlaying && static_cast<size_t>(i) == currentLogicalIndex);
            std::string marker = isCurrent ? "> " : "";
            drawText(marker + std::to_string(i + 1), x + 10, itemY + 15, m_theme.textDim, 12);
            
            // Title
            std::string title = media.getTitle().empty() ? media.getFileName() : media.getTitle();
            if (title.length() > 50) title = title.substr(0, 47) + "...";
            uint32_t titleColor = isCurrent ? m_theme.success : m_theme.textPrimary;
            drawText(title, x + 40, itemY + 15, titleColor, 14);
            
            // Artist
            std::string artist = media.getArtist();
            if (artist.length() > 30) artist = artist.substr(0, 27) + "...";
            if (!artist.empty()) {
                drawText(artist, x + w / 2, itemY + 15, m_theme.textSecondary, 12);
            }
            
            // Duration
            int duration = media.getDuration();
            if (duration > 0) {
                int mins = duration / 60;
                int secs = duration % 60;
                std::string timeStr = std::to_string(mins) + ":" + (secs < 10 ? "0" : "") + std::to_string(secs);
                drawText(timeStr, x + w - 80, itemY + 15, m_theme.textDim, 12);
            }
            
            // Remove button (right side)
            int removeBtnX = x + w - 30;
            if (isMouseOver(removeBtnX, itemY + 5, 20, 20) && mouseInList) {
                drawText("x", removeBtnX + 6, itemY + 14, m_theme.primaryHover, 14);
                if (m_mouseClicked && !m_state.showContextMenu && !m_state.showAddToPlaylistDialog && !m_state.showPropertiesDialog) {
                    m_queueController->removeByPath(media.getFilePath());
                    break;
                }
            } else {
                if (hover) drawText("x", removeBtnX + 6, itemY + 14, m_theme.textDim, 14);
            }
            
            // Click to Play (Jump to this item)
            // Left click on item (not remove btn)
            if (hover && m_mouseClicked && m_mouseX < removeBtnX && !m_state.showContextMenu && !m_state.showAddToPlaylistDialog && !m_state.showPropertiesDialog) {
                 if (m_playbackController) {
                     m_playbackController->playItemAt(i);
                     
                     // Cập nhật UI state trực tiếp để tránh delay 1 frame
                     std::string title = media.getTitle().empty() ? media.getFileName() : media.getTitle();
                     std::string artist = media.getArtist().empty() ? "Unknown Artist" : media.getArtist();
                     m_state.currentTrackTitle = title;
                     m_state.currentTrackArtist = artist;
                     m_state.isPlaying = true;
                     
                     m_mouseClicked = false;
                 }
            }
        }
        
        SDL_RenderSetClipRect(m_renderer, nullptr);
    }
}

void ImGuiManager::renderPlaylistPanel() {
    int x = SIDEBAR_WIDTH + 10;
    int y = MENU_BAR_HEIGHT + 10;
    int w = m_width - SIDEBAR_WIDTH - 20;
    
    // Header
    drawText("Playlists", x, y, m_theme.textPrimary, 20);
    y += 40;
    
    // New Playlist button
    int btnW = 150;
    int btnH = 35;
    bool btnHover = isMouseOver(x, y, btnW, btnH);
    drawRect(x, y, btnW, btnH, btnHover ? m_theme.primaryHover : m_theme.primary);
    drawText("+ New Playlist", x + 15, y + 9, m_theme.textPrimary, 14);
    y += 50;
    
    // Playlist list section
    if (m_state.showCreatePlaylistDialog) {
        // Dialog Overlay
        drawRect(x + w/2 - 150, y, 300, 150, m_theme.surface);
        drawRect(x + w/2 - 150, y, 300, 150, m_theme.border, false); // Border
        
        drawText("Create New Playlist", x + w/2 - 130, y + 20, m_theme.textPrimary, 16);
        
        // Input box
        drawRect(x + w/2 - 130, y + 50, 260, 30, m_theme.background);
        drawText(m_state.newPlaylistName.empty() ? "Enter name..." : m_state.newPlaylistName, 
                 x + w/2 - 120, y + 58, m_theme.textPrimary, 14);
        // Cursor
        if (SDL_GetTicks() % 1000 < 500) {
            int textW = m_state.newPlaylistName.length() * 8; // Approx width
            drawRect(x + w/2 - 120 + textW, y + 55, 2, 20, m_theme.textPrimary);
        }
        
        // Buttons
        // Create
        bool createHover = isMouseOver(x + w/2 + 20, y + 100, 100, 30);
        drawRect(x + w/2 + 20, y + 100, 100, 30, createHover ? m_theme.primaryHover : m_theme.primary);
        drawText("Create", x + w/2 + 45, y + 107, m_theme.textPrimary, 14);
        if (createHover && m_mouseClicked && !m_state.newPlaylistName.empty() && m_playlistController) {
            m_playlistController->createPlaylist(m_state.newPlaylistName);
            m_state.showCreatePlaylistDialog = false;
            m_state.newPlaylistName.clear();
            SDL_StopTextInput();
            m_mouseClicked = false; // Consume click
        }
        
        // Cancel
        bool cancelHover = isMouseOver(x + w/2 - 120, y + 100, 100, 30);
        drawRect(x + w/2 - 120, y + 100, 100, 30, cancelHover ? m_theme.surfaceHover : m_theme.surfaceActive);
        drawText("Cancel", x + w/2 - 95, y + 107, m_theme.textPrimary, 14);
        if (cancelHover && m_mouseClicked) {
            m_state.showCreatePlaylistDialog = false;
            m_state.newPlaylistName.clear();
            SDL_StopTextInput();
            m_mouseClicked = false;
        }
        
        return; // Don't match clicks below
    } else {
        // Handle "New Playlist" button click from header (simulated here for flow)
        if (isMouseOver(x, y - 50, 150, 35) && m_mouseClicked) {
             m_state.showCreatePlaylistDialog = true;
             SDL_StartTextInput();
             m_mouseClicked = false;
        }
    }
    
    if (m_playlistController) {
        if (!m_state.selectedPlaylistId.empty()) {
            auto playlistOpt = m_playlistController->getPlaylistById(m_state.selectedPlaylistId);
            if (!playlistOpt) {
                m_state.selectedPlaylistId.clear(); // Playlist deleted?
                return;
            }
            const auto& playlist = *playlistOpt;
            
            // Header: < Back | Playlist Name | Rename
            if (isMouseOver(x, y, 60, 30)) {
                drawRect(x, y, 60, 30, m_theme.surfaceHover);
                if (m_mouseClicked) {
                    m_state.selectedPlaylistId.clear();
                    m_mouseClicked = false;
                    return;
                }
            }
            drawText("< Back", x + 10, y + 8, m_theme.textPrimary, 14);
            
            drawText(playlist.getName(), x + 80, y + 5, m_theme.textPrimary, 20);
            
            // Rename Button
            int renameX = x + w - 80;
            if (isMouseOver(renameX, y, 80, 30)) {
                drawRect(renameX, y, 80, 30, m_theme.surfaceHover);
                if (m_mouseClicked) {
                    // Trigger rename dialog
                    m_state.showRenamePlaylistDialog = true;
                    m_state.renamePlaylistId = playlist.getId();
                    m_state.renamePlaylistName = playlist.getName();
                    SDL_StartTextInput();
                    m_mouseClicked = false;
                }
            }
            drawText("Rename", renameX + 15, y + 8, m_theme.textPrimary, 12);
            
            y += 50;
            
            // List Items
            const auto& items = playlist.getItems();
            if (items.empty()) {
                drawText("Playlist is empty. Add songs from Library.", x, y + 20, m_theme.textDim, 14);
            }
            
            int itemY = y;
            for (size_t i = 0; i < items.size(); ++i) {
                if (itemY > m_height - PLAYER_BAR_HEIGHT - 40) break; // Clip
                const auto& media = items[i];
                
                bool hover = isMouseOver(x, itemY, w, ITEM_HEIGHT);
                bool fileExists = std::filesystem::exists(media.getFilePath());
                uint32_t bg = (i % 2 == 0) ? m_theme.background : m_theme.surface;
                if (hover) bg = m_theme.surfaceHover;
                
                drawRect(x, itemY, w, ITEM_HEIGHT, bg);
                
                drawText(std::to_string(i+1) + ".", x + 10, itemY + 15, m_theme.textSecondary, 12);
                
                std::string title = media.getTitle().empty() ? media.getFileName() : media.getTitle();
                if (!fileExists) title += " (file not found)";
                if (title.length() > 40) title = title.substr(0, 37) + "...";
                uint32_t titleColor = fileExists ? m_theme.textPrimary : m_theme.textDim;
                drawText(title, x + 40, itemY + 15, titleColor, 14);
                
                // Context Menu (Right Click)
                int mx, my;
                uint32_t buttons = SDL_GetMouseState(&mx, &my);
                if (hover && (buttons & SDL_BUTTON(SDL_BUTTON_RIGHT))) {
                     m_state.showContextMenu = true;
                     m_state.contextMenuX = mx;
                     m_state.contextMenuY = my;
                     m_state.contextMediaItem = media;
                     m_state.contextMenuSource = ui::ContextMenuSource::Playlist;
                }

                // Play Button (Double click or icon?)
                // Click to play (Left Click)
                if (hover && m_mouseClicked && !m_state.showContextMenu && !m_state.showAddToPlaylistDialog && !m_state.showPropertiesDialog && !m_state.showRenamePlaylistDialog && m_mouseX < x + w - 40 && !(buttons & SDL_BUTTON(SDL_BUTTON_RIGHT))) {
                    // Check if file exists before attempting to play
                    if (fileExists) {
                        // Play this item OR play whole playlist starting here?
                        // Standard logic: Play playlist starting at index.
                        // Need QueueController->addPlaylistToQueue(playlist) then play index?
                        // Or Replace Queue with Playlist?
                        // Let's just Add item to queue and play for now.
                        if (m_queueController && m_playbackController) {
                            // Clear queue and play playlist? Or just append?
                            // Usually "Play Playlist" replaces queue.
                            m_queueController->clearQueue();
                            m_queueController->addPlaylistToQueue(playlist);
                            m_queueController->jumpToIndex(i); 
                            m_playbackController->play(); 
                        }
                    }
                    // File doesn't exist - do nothing (visual feedback already shown)
                    m_mouseClicked = false;
                }
                
                // Remove Button
                int remX = x + w - 30;
                if (isMouseOver(remX, itemY + 5, 20, 20)) {
                    drawText("x", remX + 6, itemY + 14, m_theme.error, 14);
                    if (m_mouseClicked && !m_state.showContextMenu && !m_state.showAddToPlaylistDialog && !m_state.showRenamePlaylistDialog) {
                        m_playlistController->removeMediaFromPlaylist(playlist.getId(), i);
                        break; // List modified
                    }
                } else if (hover) {
                    drawText("x", remX + 6, itemY + 14, m_theme.textDim, 14);
                }
                
                itemY += ITEM_HEIGHT;
            }
            
        } else {
            // LIST VIEW
            auto playlists = m_playlistController->getAllPlaylists();
            
            int itemY = y;
            for (const auto& playlist : playlists) {
                bool hover = isMouseOver(x, itemY, w, ITEM_HEIGHT);
                
                drawRect(x, itemY, w, ITEM_HEIGHT, hover ? m_theme.surfaceHover : m_theme.surface);
                drawText("#", x + 15, itemY + 15, m_theme.primary, 14);
                drawText(playlist.getName(), x + 40, itemY + 15, m_theme.textPrimary, 14);
                
                std::string countStr = std::to_string(playlist.getItemCount()) + " tracks";
                drawText(countStr, x + w - 100, itemY + 15, m_theme.textDim, 12);
                
                // Click to Enter Detail View (if not clicking delete)
                if (hover && m_mouseClicked && m_mouseX < x + w - 40) {
                    m_state.selectedPlaylistId = playlist.getId();
                    m_mouseClicked = false;
                }
                
                // Delete button
                int delX = x + w - 30;
                if (isMouseOver(delX, itemY + 5, 20, 20)) {
                     drawText("x", delX + 6, itemY + 14, m_theme.primaryHover, 14);
                     if (m_mouseClicked) {
                         m_playlistController->deletePlaylist(playlist.getId());
                         break; // Modifying list
                     }
                } else {
                     if (hover) drawText("x", delX + 6, itemY + 14, m_theme.textDim, 14);
                }
                
                itemY += ITEM_HEIGHT + 5;
            }
        }
    }
}

void ImGuiManager::renderHistoryPanel() {
    int x = SIDEBAR_WIDTH + 10;
    int y = MENU_BAR_HEIGHT + 10;
    int w = m_width - SIDEBAR_WIDTH - 20;
    
    // Header
    drawText("History", x, y, m_theme.textPrimary, 20);
    y += 40;
    
    // Recently Played section
    drawText("Recently Played", x, y, m_theme.textSecondary, 14);
    y += 25;
    
    // Check if we have history
    if (m_state.currentTrackTitle.empty()) {
        drawRect(x, y, w, 100, m_theme.surface);
        drawText("No playback history yet", x + 20, y + 40, m_theme.textDim, 14);
        drawText("Play some tracks to see them here", x + 20, y + 60, m_theme.textDim, 12);
    } else {
        // Show current/last played track
        drawRect(x, y, w, 60, m_theme.surface);
        drawText(">", x + 15, y + 18, m_theme.success, 18);
        drawText(m_state.currentTrackTitle, x + 50, y + 12, m_theme.textPrimary, 16);
        drawText(m_state.currentTrackArtist, x + 50, y + 35, m_theme.textSecondary, 12);
        drawText("Now Playing", x + w - 100, y + 20, m_theme.success, 12);
        y += 70;
    }
    
    // History List (with scroll - supports up to 100 items)
    if (!m_historyList.empty()) {
        int listAreaH = m_height - MENU_BAR_HEIGHT - PLAYER_BAR_HEIGHT - y - 20;
        int maxScroll = std::max(0, static_cast<int>(m_historyList.size() * ITEM_HEIGHT) - listAreaH);
        if (m_state.historyScrollOffset > maxScroll) m_state.historyScrollOffset = maxScroll;
        
        SDL_Rect listClip = {x, y, w, listAreaH};
        SDL_RenderSetClipRect(m_renderer, &listClip);
        
        int itemY = y - m_state.historyScrollOffset;
        int index = 0;
        
        for (const auto& entry : m_historyList) {
            if (itemY + ITEM_HEIGHT < y || itemY > m_height - PLAYER_BAR_HEIGHT - 20) {
                itemY += ITEM_HEIGHT;
                index++;
                continue;
            }
            
            bool mouseInList = isMouseOver(x, y, w, listAreaH);
            bool hover = mouseInList && isMouseOver(x, itemY, w, ITEM_HEIGHT);
             
             // Alternating background
            uint32_t bg = (index % 2 == 0) ? m_theme.background : m_theme.surface;
            if (hover) bg = m_theme.surfaceHover;
            
            drawRect(x, itemY, w, ITEM_HEIGHT, bg);
            
            // Serial number (1-based)
            drawText(std::to_string(index + 1), x + 15, itemY + 15, m_theme.textDim, 14);
            
            bool fileExists = std::filesystem::exists(entry.media.getFilePath());
            std::string title = entry.media.getTitle().empty() ? entry.media.getFileName() : entry.media.getTitle();
            if (!fileExists) title += " (file not found)";
            if (title.length() > 50) title = title.substr(0, 47) + "...";
            uint32_t titleColor = fileExists ? m_theme.textPrimary : m_theme.textDim;
            drawText(title, x + 50, itemY + 15, titleColor, 14);
            
            std::string artist = entry.media.getArtist();
            if (!artist.empty()) drawText(artist, x + w / 2, itemY + 15, fileExists ? m_theme.textSecondary : m_theme.textDim, 14);
            
            // Context Menu (Right Click)
            int mx, my;
            uint32_t buttons = SDL_GetMouseState(&mx, &my);
            if (hover && (buttons & SDL_BUTTON(SDL_BUTTON_RIGHT))) {
                 m_state.showContextMenu = true;
                 m_state.contextMenuX = mx;
                 m_state.contextMenuY = my;
                 m_state.contextMediaItem = entry.media;
                 m_state.contextMenuSource = ui::ContextMenuSource::Queue;
            }
            
            // Click to Play (Left Click only)
             if (hover && m_mouseClicked && !m_state.showContextMenu && !m_state.showAddToPlaylistDialog && !m_state.showPropertiesDialog && m_queueController && m_playbackController && !(buttons & SDL_BUTTON(SDL_BUTTON_RIGHT))) {
                m_queueController->addToQueue(entry.media);
                // Play the newly added item (at the end)
                if (m_queueController->getQueueSize() > 0) {
                     size_t newIndex = m_queueController->getQueueSize() - 1;
                     m_playbackController->playItemAt(newIndex);
                }
                 m_mouseClicked = false;
             }

            itemY += ITEM_HEIGHT;
            index++;
        }
        
        SDL_RenderSetClipRect(m_renderer, nullptr);
    } else {
        drawText("No history available.", x, y + 20, m_theme.textDim, 14);
    }
}

void ImGuiManager::renderScanProgress(const std::string& path, int current, int total) {
    int boxW = 450;
    int boxH = 170;
    int boxX = (m_width - boxW) / 2;
    int boxY = (m_height - boxH) / 2;

    m_state.scanDialogVisible = true;
    
    // Dimmed overlay
    drawRect(0, 0, m_width, m_height, 0x80000000);
    
    // Dialog box
    drawRect(boxX, boxY, boxW, boxH, m_theme.surface);
    drawRect(boxX, boxY, boxW, boxH, m_theme.border, false);
    
    // Title
    drawText("Scanning Media Files", boxX + 20, boxY + 15, m_theme.textPrimary, 18);
    
    // Path
    std::string shortPath = path;
    if (shortPath.length() > 45) {
        shortPath = "..." + shortPath.substr(shortPath.length() - 42);
    }
    drawText(shortPath, boxX + 20, boxY + 50, m_theme.textDim, 12);
    
    // Progress bar
    float progress = total > 0 ? static_cast<float>(current) / total : 0;
    drawProgressBar(boxX + 20, boxY + 80, boxW - 40, 12, progress, 
                    m_theme.primary, m_theme.scrollbar);
    
    // Count
    std::string countText = std::to_string(current) + " files found";
    drawText(countText, boxX + 20, boxY + 105, m_theme.textSecondary, 12);
    
    // Cancel button
    int cancelBtnW = 100;
    int cancelBtnH = 30;
    int cancelBtnX = boxX + (boxW - cancelBtnW) / 2;
    int cancelBtnY = boxY + 130;
    
    bool cancelHover = isMouseOver(cancelBtnX, cancelBtnY, cancelBtnW, cancelBtnH);
    drawRect(cancelBtnX, cancelBtnY, cancelBtnW, cancelBtnH, 
             cancelHover ? m_theme.error : m_theme.surfaceHover);
    drawText("Cancel", cancelBtnX + 28, cancelBtnY + 7, m_theme.textPrimary, 14);
    
    if (cancelHover && m_modalMouseClicked && m_onCancelScan)
    {
        m_onCancelScan();
        m_modalMouseClicked = false;
    }
}

void ImGuiManager::showUsbPopup(const std::string& path) {
    m_state.showUsbDialog = true;
    m_state.usbPath = path;
}

void ImGuiManager::renderPathInputScreen(const std::string& currentPathPlaceholder) {
    int boxW = 520;
    int boxH = 180;
    int boxX = (m_width - boxW) / 2;
    int boxY = (m_height - boxH) / 2;
    
    drawRect(0, 0, m_width, m_height, 0x80000000);
    drawRect(boxX, boxY, boxW, boxH, m_theme.surface);
    drawRect(boxX, boxY, boxW, boxH, m_theme.border, false);
    
    drawText("Chọn thư mục thư viện", boxX + 20, boxY + 15, m_theme.textPrimary, 18);
    
    int inputY = boxY + 50;
    int inputH = 32;
    drawRect(boxX + 20, inputY, boxW - 40, inputH, m_theme.background);
    drawRect(boxX + 20, inputY, boxW - 40, inputH, m_theme.border, false);
    
    std::string displayText = m_state.libraryPathInput.empty() ? currentPathPlaceholder : m_state.libraryPathInput;
    if (displayText.length() > 58) displayText = "..." + displayText.substr(displayText.length() - 55);
    uint32_t textColor = m_state.libraryPathInput.empty() ? m_theme.textDim : m_theme.textPrimary;
    drawText(displayText, boxX + 28, inputY + 8, textColor, 14);
    
    bool inputHover = isMouseOver(boxX + 20, inputY, boxW - 40, inputH);
    if (inputHover && m_mouseClicked) {
        m_state.focusPathInput = true;
        SDL_StartTextInput();
        m_mouseClicked = false;
    }
    if (!inputHover && m_mouseClicked) {
        m_state.focusPathInput = false;
        SDL_StopTextInput();
    }
    
    int btnY = boxY + 100;
    int btnW = 120;
    int btnH = 36;
    bool scanHover = isMouseOver(boxX + boxW - 20 - btnW, btnY, btnW, btnH);
    drawRect(boxX + boxW - 20 - btnW, btnY, btnW, btnH, scanHover ? m_theme.primaryHover : m_theme.primary);
    drawText("Quét", boxX + boxW - 20 - btnW + 42, btnY + 10, m_theme.textPrimary, 14);
    if (scanHover && m_mouseClicked && m_onRequestScan) {
        m_onRequestScan(m_state.libraryPathInput.empty() ? currentPathPlaceholder : m_state.libraryPathInput);
        m_mouseClicked = false;
    }
    
    if (!m_state.libraryPathError.empty()) {
        std::string err = m_state.libraryPathError;
        if (err.length() > 60) err = err.substr(0, 57) + "...";
        drawText(err, boxX + 20, boxY + 148, m_theme.error, 12);
    }
}

void ImGuiManager::setControllers(
    std::shared_ptr<controllers::PlaybackController> playback,
    std::shared_ptr<controllers::QueueController> queue,
    std::shared_ptr<controllers::LibraryController> library,
    std::shared_ptr<controllers::PlaylistController> playlist
) {
    m_playbackController = std::move(playback);
    m_queueController = std::move(queue);
    m_libraryController = std::move(library);
    m_playlistController = std::move(playlist);
}

void ImGuiManager::setMediaList(const std::vector<models::MediaFileModel>* mediaList) {
    m_mediaList = mediaList;
}

void ImGuiManager::setHistoryList(const std::vector<repositories::PlaybackHistoryEntry>& historyList) {
    m_historyList = historyList;
}

// Drawing helpers

void ImGuiManager::drawRect(int x, int y, int w, int h, uint32_t color, bool filled) {
    Uint8 r, g, b, a;
    unpackColor(color, r, g, b, a);
    SDL_SetRenderDrawColor(m_renderer, r, g, b, a);
    
    SDL_Rect rect = {x, y, w, h};
    if (filled) {
        SDL_RenderFillRect(m_renderer, &rect);
    } else {
        SDL_RenderDrawRect(m_renderer, &rect);
    }
}

void ImGuiManager::drawRoundedRect(int x, int y, int w, int h, int radius, uint32_t color, bool filled) {
    // Simplified - just draw normal rect
    drawRect(x, y, w, h, color, filled);
}

void ImGuiManager::drawText(const std::string& text, int x, int y, uint32_t color, int fontSize) {
    if (!m_font || text.empty()) return;
    
    TTF_Font* font = m_font;
    if (fontSize >= 18 && m_fontLarge) {
        font = m_fontLarge;
    } else if (fontSize <= 12 && m_fontSmall) {
        font = m_fontSmall;
    }
    
    Uint8 r, g, b, a;
    unpackColor(color, r, g, b, a);
    SDL_Color sdlColor = {r, g, b, a};
    
    SDL_Surface* surface = TTF_RenderUTF8_Blended(font, text.c_str(), sdlColor);
    if (!surface) return;
    
    SDL_Texture* texture = SDL_CreateTextureFromSurface(m_renderer, surface);
    if (texture) {
        SDL_Rect destRect = {x, y, surface->w, surface->h};
        SDL_RenderCopy(m_renderer, texture, nullptr, &destRect);
        SDL_DestroyTexture(texture);
    }
    
    SDL_FreeSurface(surface);
}

void ImGuiManager::drawIcon(const std::string& icon, int x, int y, uint32_t color, int size) {
    drawText(icon, x, y, color, size);
}

void ImGuiManager::drawProgressBar(int x, int y, int w, int h, float progress, uint32_t fg, uint32_t bg) {
    // Background
    drawRect(x, y, w, h, bg);
    
    // Progress
    int progressW = static_cast<int>(w * std::clamp(progress, 0.0f, 1.0f));
    if (progressW > 0) {
        drawRect(x, y, progressW, h, fg);
    }
}

void ImGuiManager::drawButton(const std::string& label, int x, int y, int w, int h, bool active) {
    bool hover = isMouseOver(x, y, w, h);
    uint32_t bg = active ? m_theme.primary : (hover ? m_theme.surfaceHover : m_theme.surface);
    
    drawRect(x, y, w, h, bg);
    drawRect(x, y, w, h, m_theme.border, false);
    
    // Center text
    int textX = x + 10;
    int textY = y + (h - 14) / 2;
    drawText(label, textX, textY, m_theme.textPrimary, 14);
}

void ImGuiManager::drawSlider(float* value, int x, int y, int w, int h) {
    drawRect(x, y + h/2 - 2, w, 4, m_theme.scrollbar);
    
    int knobX = x + static_cast<int>(w * (*value)) - 4;
    drawRect(knobX, y, 8, h, m_theme.primary);
    
    if (isMouseOver(x, y, w, h) && m_mouseDown) {
        *value = std::clamp(static_cast<float>(m_mouseX - x) / w, 0.0f, 1.0f);
    }
}

void ImGuiManager::renderOverlays() {
    // 0a. USB Connected Dialog (High Priority)
    if (m_state.showUsbDialog) {
        int boxW = 400;
        int boxH = 160;
        int boxX = (m_width - boxW) / 2;
        int boxY = (m_height - boxH) / 2;
        
        drawRect(0, 0, m_width, m_height, 0x80000000); // Dim background
        drawRect(boxX, boxY, boxW, boxH, m_theme.surface);
        drawRect(boxX, boxY, boxW, boxH, m_theme.border, false);
        
        drawText("USB Connected", boxX + 20, boxY + 20, m_theme.textPrimary, 18);
        
        std::string msg = "Detected USB drive at:";
        drawText(msg, boxX + 20, boxY + 50, m_theme.textSecondary, 14);
        
        std::string pathDisp = m_state.usbPath;
        if (pathDisp.length() > 45) pathDisp = "..." + pathDisp.substr(pathDisp.length() - 42);
        drawText(pathDisp, boxX + 20, boxY + 70, m_theme.primary, 14);
        
        // Buttons
        int btnY = boxY + 110;
        
        // Change Source Button
        int changeW = 140;
        int changeH = 35;
        int changeX = boxX + boxW - 20 - changeW;
        bool changeHover = isMouseOver(changeX, btnY, changeW, changeH);
        drawRect(changeX, btnY, changeW, changeH, changeHover ? m_theme.primaryHover : m_theme.primary);
        drawText("Change Source", changeX + 15, btnY + 9, m_theme.textPrimary, 14);
        
        if (changeHover && m_modalMouseClicked) { // Use modal click flag
            if (m_onRequestScan) {
                m_onRequestScan(m_state.usbPath);
            }
            m_state.showUsbDialog = false;
            m_modalMouseClicked = false;
        }
        
        // Close (X) Button - top right
        int closeSize = 30;
        int closeX = boxX + boxW - 30;
        int closeY = boxY;
        bool closeHover = isMouseOver(closeX, closeY, closeSize, closeSize);
        if (closeHover) drawRect(closeX, closeY, closeSize, closeSize, m_theme.error);
        drawText("X", closeX + 10, closeY + 5, m_theme.textPrimary, 14);
        
        if (closeHover && m_modalMouseClicked) { // Use modal click flag
             m_state.showUsbDialog = false;
             m_modalMouseClicked = false;
        }
    }

    // 0. Change library path dialog (Settings)
    if (m_state.showChangePathDialog) {
        int boxW = 520;
        int boxH = 200;
        int boxX = (m_width - boxW) / 2;
        int boxY = (m_height - boxH) / 2;
        drawRect(0, 0, m_width, m_height, 0x80000000);
        // Close modal when clicking outside the dialog area
        bool clickOutside = m_modalMouseClicked && !isMouseOver(boxX, boxY, boxW, boxH);
        if (clickOutside)
        {
            m_state.showChangePathDialog = false;
            m_state.focusPathInput = false;
            SDL_StopTextInput();
            m_modalMouseClicked = false;
            return;
        }
        drawRect(boxX, boxY, boxW, boxH, m_theme.surface);
        drawRect(boxX, boxY, boxW, boxH, m_theme.border, false);
        drawText("Đổi thư mục thư viện", boxX + 20, boxY + 15, m_theme.textPrimary, 18);
        int inputY = boxY + 50;
        int inputH = 32;
        drawRect(boxX + 20, inputY, boxW - 40, inputH, m_theme.background);
        drawRect(boxX + 20, inputY, boxW - 40, inputH, m_theme.border, false);
        std::string disp = m_state.libraryPathInput.empty() ? "(đường dẫn)" : m_state.libraryPathInput;
        if (disp.length() > 58) disp = "..." + disp.substr(disp.length() - 55);
        drawText(disp, boxX + 28, inputY + 8, m_state.libraryPathInput.empty() ? m_theme.textDim : m_theme.textPrimary, 14);
        bool inputHover = isMouseOver(boxX + 20, inputY, boxW - 40, inputH);
        if (inputHover && m_modalMouseClicked)
        {
            m_state.focusPathInput = true;
            SDL_StartTextInput();
            m_modalMouseClicked = false;
        }
        int btnY = boxY + 100;
        int apW = 100, huW = 80, btnH = 36;
        bool apHover = isMouseOver(boxX + boxW - 20 - apW - 10 - huW, btnY, apW, btnH);
        bool huHover = isMouseOver(boxX + boxW - 20 - huW, btnY, huW, btnH);
        drawRect(boxX + boxW - 20 - apW - 10 - huW, btnY, apW, btnH, apHover ? m_theme.primaryHover : m_theme.primary);
        drawText("Scan", boxX + boxW - 20 - apW - 10 - huW + 28, btnY + 10, m_theme.textPrimary, 14);
        drawRect(boxX + boxW - 20 - huW, btnY, huW, btnH, huHover ? m_theme.surfaceHover : m_theme.surfaceActive);
        drawText("Cancel", boxX + boxW - 20 - huW + 22, btnY + 10, m_theme.textPrimary, 14);
        if (apHover && m_modalMouseClicked && m_onChangeLibraryPath)
        {
            m_onChangeLibraryPath(m_state.libraryPathInput.empty() ? "" : m_state.libraryPathInput);
            m_modalMouseClicked = false;
        }
        if (huHover && m_modalMouseClicked)
        {
            m_state.showChangePathDialog = false;
            m_state.focusPathInput = false;
            SDL_StopTextInput();
            m_modalMouseClicked = false;
        }
        if (!m_state.libraryPathError.empty()) {
            std::string err = m_state.libraryPathError;
            if (err.length() > 60) err = err.substr(0, 57) + "...";
            drawText(err, boxX + 20, boxY + 168, m_theme.error, 12);
        }
    }
    
    // 1. Context Menu
    if (m_state.showContextMenu) {
        int menuW = 150;
        int menuH = 140; // 4 items (35 each)
        
        // Draw Menu Background
        drawRect(m_state.contextMenuX, m_state.contextMenuY, menuW, menuH, m_theme.surfaceHover);
        drawRect(m_state.contextMenuX, m_state.contextMenuY, menuW, menuH, m_theme.border, false); // Border
        
        // Item 1: Add to Playlist
        int y = m_state.contextMenuY;
        if (isMouseOver(m_state.contextMenuX, y, menuW, 35)) {
            drawRect(m_state.contextMenuX, y, menuW, 35, m_theme.primary);
            if (m_mouseClicked) {
                m_state.showContextMenu = false;
                m_state.showAddToPlaylistDialog = true;
                m_mouseClicked = false;
            }
        }
        drawText("Add to Playlist", m_state.contextMenuX + 10, y + 8, m_theme.textPrimary, 14);

        // Item 2: Add to Queue
        y += 35;
        if (isMouseOver(m_state.contextMenuX, y, menuW, 35)) {
            drawRect(m_state.contextMenuX, y, menuW, 35, m_theme.primary);
            if (m_mouseClicked) {
                 if (m_queueController) {
                    m_queueController->addToQueue(m_state.contextMediaItem);
                 }
                m_state.showContextMenu = false;
                m_mouseClicked = false;
            }
        }
        drawText("Add to Queue", m_state.contextMenuX + 10, y + 8, m_theme.textPrimary, 14);
        
        // Item 3: Play Next
        y += 35;
        if (isMouseOver(m_state.contextMenuX, y, menuW, 35)) {
            drawRect(m_state.contextMenuX, y, menuW, 35, m_theme.primary);
            if (m_mouseClicked) {
                 if (m_queueController) {
                    m_queueController->addToQueueNext(m_state.contextMediaItem);
                 }
                m_state.showContextMenu = false;
                m_mouseClicked = false;
            }
        }
        drawText("Play Next", m_state.contextMenuX + 10, y + 8, m_theme.textPrimary, 14);
        
        // Item 4: Properties
        y += 35;
        if (isMouseOver(m_state.contextMenuX, y, menuW, 35)) {
            drawRect(m_state.contextMenuX, y, menuW, 35, m_theme.primary);
            if (m_mouseClicked) {
                m_state.showContextMenu = false;
                m_state.showPropertiesDialog = true;
                const auto& media = m_state.contextMediaItem;
                m_state.metadataEdit.filePath = media.getFilePath();
                m_state.metadataEdit.fileName = media.getFileName();
                m_state.metadataEdit.extension = media.getExtension();
                m_state.metadataEdit.typeStr = media.isAudio() ? "Audio" : (media.isVideo() ? "Video" : "Unknown");
                size_t sz = media.getFileSize();
                if (sz >= 1024 * 1024)
                    m_state.metadataEdit.fileSizeStr = std::to_string(sz / (1024 * 1024)) + " MB";
                else if (sz >= 1024)
                    m_state.metadataEdit.fileSizeStr = std::to_string(sz / 1024) + " KB";
                else
                    m_state.metadataEdit.fileSizeStr = std::to_string(sz) + " B";
                int dur = media.getDuration();
                m_state.metadataEdit.durationStr = (dur > 0) ? (std::to_string(dur / 60) + ":" + (dur % 60 < 10 ? "0" : "") + std::to_string(dur % 60)) : "-";
                m_state.metadataEdit.title = media.getTitle().empty() ? media.getFileName() : media.getTitle();
                m_state.metadataEdit.artist = media.getArtist().empty() ? "-" : media.getArtist();
                m_state.metadataEdit.album = media.getAlbum().empty() ? "-" : media.getAlbum();
                m_state.metadataEdit.genre = "-";
                m_state.metadataEdit.year = "-";
                m_state.metadataEdit.publisher = "-";
                m_state.metadataEdit.bitrateStr = "-";
                if (m_getMetadataForProperties) {
                    auto meta = m_getMetadataForProperties(media.getFilePath());
                    if (meta) {
                        if (!meta->getTitle().empty()) m_state.metadataEdit.title = meta->getTitle();
                        if (!meta->getArtist().empty()) m_state.metadataEdit.artist = meta->getArtist();
                        if (!meta->getAlbum().empty()) m_state.metadataEdit.album = meta->getAlbum();
                        if (!meta->getGenre().empty()) m_state.metadataEdit.genre = meta->getGenre();
                        if (!meta->getYear().empty()) m_state.metadataEdit.year = meta->getYear();
                        if (!meta->getPublisher().empty()) m_state.metadataEdit.publisher = meta->getPublisher();
                        m_state.metadataEdit.durationStr = meta->getFormattedDuration();
                        if (meta->getDuration() <= 0 && dur > 0)
                            m_state.metadataEdit.durationStr = std::to_string(dur / 60) + ":" + (dur % 60 < 10 ? "0" : "") + std::to_string(dur % 60);
                        if (meta->getBitrate() > 0) m_state.metadataEdit.bitrateStr = std::to_string(meta->getBitrate()) + " kbps";
                    }
                }
                m_mouseClicked = false;
            }
        }
        drawText("Properties", m_state.contextMenuX + 10, y + 8, m_theme.textPrimary, 14);
        
        // Close if clicked outside
        if (m_mouseClicked && !isMouseOver(m_state.contextMenuX, m_state.contextMenuY, menuW, menuH)) {
            m_state.showContextMenu = false;
        }
    }
    
    // 2. Add to Playlist Dialog
    if (m_state.showAddToPlaylistDialog) {
        int dlgW = 400;
        int dlgH = 300;
        int x = (m_width - dlgW) / 2;
        int y = (m_height - dlgH) / 2;
        
        drawRect(x, y, dlgW, dlgH, m_theme.surface);
        drawRect(x, y, dlgW, dlgH, m_theme.border, false); // Border
        
        drawText("Add to Playlist", x + 20, y + 20, m_theme.textPrimary, 18);
        drawText("Select a playlist:", x + 20, y + 50, m_theme.textSecondary, 14);
        
        // List Playlists
        if (m_playlistController) {
            auto playlists = m_playlistController->getAllPlaylists();
            int listY = y + 80;
            for (const auto& pl : playlists) {
                if (listY > y + dlgH - 60) break;
                
                if (isMouseOver(x + 20, listY, dlgW - 40, 30)) {
                    drawRect(x + 20, listY, dlgW - 40, 30, m_theme.surfaceHover);
                    if (m_mouseClicked) {
                        // Add to this playlist
                         m_playlistController->addMediaToPlaylist(pl.getId(), m_state.contextMediaItem);
                        m_state.showAddToPlaylistDialog = false;
                        m_mouseClicked = false;
                    }
                }
                drawText(pl.getName(), x + 30, listY + 6, m_theme.textPrimary, 14);
                listY += 35;
            }
        }
        
        // Cancel Button
        if (isMouseOver(x + dlgW - 100, y + dlgH - 40, 80, 30)) {
             drawRect(x + dlgW - 100, y + dlgH - 40, 80, 30, m_theme.surfaceActive);
             if (m_mouseClicked) {
                 m_state.showAddToPlaylistDialog = false;
                 m_mouseClicked = false;
             }
        } else {
             drawRect(x + dlgW - 100, y + dlgH - 40, 80, 30, m_theme.surfaceHover);
        }
        drawText("Cancel", x + dlgW - 85, y + dlgH - 33, m_theme.textPrimary, 14);
    }
    
    // 3. Properties Dialog (read-only, all metadata fields)
    if (m_state.showPropertiesDialog) {
        int dlgW = 420;
        int dlgH = 520;
        int x = (m_width - dlgW) / 2;
        int y = (m_height - dlgH) / 2;
        
        drawRect(x, y, dlgW, dlgH, m_theme.surface);
        drawRect(x, y, dlgW, dlgH, m_theme.border, false);
        
        drawText("Properties", x + 20, y + 16, m_theme.textPrimary, 18);
        
        int labelX = x + 20;
        int valueX = x + 110;
        int fieldY = y + 48;
        const int rowH = 24;
        
        auto drawRow = [&](const std::string& label, const std::string& value) {
            drawText(label + ":", labelX, fieldY, m_theme.textSecondary, 12);
            std::string v = value.empty() ? "-" : value;
            if (v.length() > 52) v = v.substr(0, 49) + "...";
            drawText(v, valueX, fieldY, m_theme.textPrimary, 12);
            fieldY += rowH;
        };
        
        drawRow("Title", m_state.metadataEdit.title);
        drawRow("Artist", m_state.metadataEdit.artist);
        drawRow("Album", m_state.metadataEdit.album);
        drawRow("Genre", m_state.metadataEdit.genre);
        drawRow("Year", m_state.metadataEdit.year);
        drawRow("Publisher", m_state.metadataEdit.publisher);
        drawRow("Duration", m_state.metadataEdit.durationStr);
        drawRow("Bitrate", m_state.metadataEdit.bitrateStr);
        drawRow("File name", m_state.metadataEdit.fileName);
        drawRow("Extension", m_state.metadataEdit.extension);
        drawRow("Type", m_state.metadataEdit.typeStr);
        drawRow("File size", m_state.metadataEdit.fileSizeStr);
        drawRow("File path", m_state.metadataEdit.filePath);

        // Close Button
        int closeY = y + dlgH - 40;
        if (isMouseOver(x + dlgW - 100, closeY, 80, 30)) {
             drawRect(x + dlgW - 100, closeY, 80, 30, m_theme.surfaceActive);
             if (m_mouseClicked) {
                 m_state.showPropertiesDialog = false;
                 m_mouseClicked = false;
             }
        } else {
             drawRect(x + dlgW - 100, closeY, 80, 30, m_theme.surfaceHover);
        }
        drawText("Close", x + dlgW - 85, closeY + 7, m_theme.textPrimary, 14);
    }

    // 4. Rename Playlist Dialog
    if (m_state.showRenamePlaylistDialog) {
        int dlgW = 400;
        int dlgH = 200;
        int x = (m_width - dlgW) / 2;
        int y = (m_height - dlgH) / 2;
        
        drawRect(x, y, dlgW, dlgH, m_theme.surface);
        drawRect(x, y, dlgW, dlgH, m_theme.border, false); // Border
        
        drawText("Rename Playlist", x + 20, y + 20, m_theme.textPrimary, 18);
        
        // Input Box
        drawRect(x + 20, y + 60, dlgW - 40, 30, m_theme.background);
        drawRect(x + 20, y + 60, dlgW - 40, 30, m_theme.primary, false);
        
        std::string display = m_state.renamePlaylistName + "_";
        drawText(display, x + 30, y + 68, m_theme.textPrimary, 14);
        
        drawText("Press Enter to save, Esc to cancel", x + 20, y + 100, m_theme.textDim, 12);
        
        // Save Button
        if (isMouseOver(x + dlgW - 180, y + dlgH - 40, 70, 30)) {
             drawRect(x + dlgW - 180, y + dlgH - 40, 70, 30, m_theme.primary);
             if (m_mouseClicked) {
                 if (!m_state.renamePlaylistName.empty() && m_playlistController) {
                     m_playlistController->renamePlaylist(m_state.renamePlaylistId, m_state.renamePlaylistName);
                 }
                 m_state.showRenamePlaylistDialog = false;
                 SDL_StopTextInput();
                 m_mouseClicked = false;
             }
        }
        drawText("Save", x + dlgW - 160, y + dlgH - 33, m_theme.textPrimary, 14);
        
        // Cancel Button - always draw background
        bool cancelHover = isMouseOver(x + dlgW - 100, y + dlgH - 40, 70, 30);
        drawRect(x + dlgW - 100, y + dlgH - 40, 70, 30, cancelHover ? m_theme.surfaceActive : m_theme.surfaceHover);
        if (cancelHover && m_mouseClicked) {
            m_state.showRenamePlaylistDialog = false;
            m_state.renamePlaylistName.clear();
            SDL_StopTextInput();
            m_mouseClicked = false;
        }
        drawText("Cancel", x + dlgW - 90, y + dlgH - 33, m_theme.textPrimary, 14);
    }
}

bool ImGuiManager::isMouseOver(int x, int y, int w, int h) const {
    return m_mouseX >= x && m_mouseX < x + w && m_mouseY >= y && m_mouseY < y + h;
}

bool ImGuiManager::isMouseClicked(int x, int y, int w, int h) const {
    return m_mouseClicked && isMouseOver(x, y, w, h);
}

bool ImGuiManager::isMouseDragging() const {
    return m_mouseDown;
}

} // namespace ui
} // namespace media_player
