// Project includes
#include "ui/ImGuiManager.h"
#include "views/IView.h"
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



Theme Theme::light() {
    Theme t;
    // Background - White
    t.background      = 0xFFFFFFFF; 
    // Surface - Very Light Blue-Gray for sidebar/panels
    t.surface         = 0xF0F4F8FF;  
    t.surfaceHover    = 0xE1E8EFFF;  
    t.surfaceActive   = 0xD0D7DEFF;  
    
    // Primary - Vibrant Blue (Google Blue / iOS Blue style)
    t.primary         = 0x1A73E8FF;  
    t.primaryHover    = 0x1557B0FF;
    t.primaryActive   = 0x174EA6FF;
    
    // Text - High Contrast
    t.textPrimary     = 0x202124FF;  // Almost Black
    t.textSecondary   = 0x5F6368FF;  // Dark Gray
    t.textDim         = 0x9AA0A6FF;  // Medium Gray
    
    // Status
    t.success         = 0x1E8E3EFF;  // Green
    t.warning         = 0xF9AB00FF;  // Yellow/Orange
    t.error           = 0xD93025FF;  // Red
    
    // UI Elements
    t.border          = 0xDADCE0FF;  // Light gray border
    t.scrollbar       = 0xF1F3F4FF;
    t.scrollbarThumb  = 0xBDC1C6FF;
    
    return t;
}

// Helper for color unpacking
static void unpackColor(uint32_t color, Uint8& r, Uint8& g, Uint8& b, Uint8& a) {
    r = (color >> 24) & 0xFF;
    g = (color >> 16) & 0xFF;
    b = (color >> 8) & 0xFF;
    a = color & 0xFF;
}

ImGuiManager::ImGuiManager() {
    // Set default theme to Light
    m_theme = Theme::light();
}

ImGuiManager::~ImGuiManager() {
    shutdown();
}

bool ImGuiManager::initialize(const std::string& title, int width, int height) {
    m_height = height;
    
    // Initialize SDL_ttf
    if (TTF_Init() < 0) {
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
        return false;
    }
    
    // Create hardware-accelerated renderer
    m_renderer = SDL_CreateRenderer(
        m_window,
        -1,
        SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC
    );
    
    if (!m_renderer) {
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
            break;
        }
    }
    
    if (!m_font) {
    }
    
    // Disable text input by default, enable only on focus
    SDL_StopTextInput();
    
    return true;
}

void ImGuiManager::registerView(NavTab tab, views::IView* view) {
    m_views[tab] = view;
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
    
    // Sync UI state from controllers (fix cho shuffle/loop và track info)
    // Only sync when not clicking to avoid overwriting pending toggle from UI buttons
    if (m_queueController && !m_mouseClicked) 
    {
        m_state.shuffleEnabled = m_queueController->isShuffleEnabled();
        m_state.loopEnabled = m_queueController->isRepeatEnabled();
        m_state.loopAllEnabled = m_queueController->isLoopAllEnabled();
    }
    
    // Sync isPlaying state from controller to ensure UI reflects actual playback state
    if (m_playbackController && !m_mouseClicked)
    {
        m_state.isPlaying = m_playbackController->isPlaying();
    }
}

void ImGuiManager::endFrame() {
    SDL_RenderPresent(m_renderer);
    
    // Reset click state after rendering so all UI elements can check it
    m_mouseClicked = false;
    m_leftMouseClicked = false;
    m_rightMouseClicked = false;
    m_modalMouseClicked = false;
}

bool ImGuiManager::processEvent(const SDL_Event& event) {
    switch (event.type) {
        case SDL_MOUSEMOTION:
            m_mouseX = event.motion.x;
            m_mouseY = event.motion.y;
            return true;
            
        case SDL_MOUSEBUTTONDOWN:
            m_mouseX = event.button.x;
            m_mouseY = event.button.y;
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
                    // Don't set m_mouseClicked here - only set it on MOUSEBUTTONUP
                    // to avoid processing click twice (once on down, once on up)
                    m_modalMouseClicked = false;
                }
                m_dragStartX = event.button.x;
                m_dragStartY = event.button.y;
            }
            return true;
            
        case SDL_MOUSEBUTTONUP:
            m_mouseX = event.button.x;
            m_mouseY = event.button.y;
            m_mouseDown = false;
            
            // Check if any modal/overlay is visible - block clicks to underlying views
            if (m_state.scanDialogVisible || m_state.showChangePathDialog || 
                m_state.showUsbDialog || m_state.showContextMenu ||
                m_state.showAddToPlaylistDialog || m_state.showPropertiesDialog ||
                m_state.showRenamePlaylistDialog) {
                m_modalMouseClicked = true;
                m_mouseClicked = false;  // Block click from reaching underlying views
                m_leftMouseClicked = false;
                m_rightMouseClicked = false;
            } else {
                m_mouseClicked = true;
                if (event.button.button == SDL_BUTTON_LEFT) m_leftMouseClicked = true;
                if (event.button.button == SDL_BUTTON_RIGHT) m_rightMouseClicked = true;
            }
            
            m_mouseDragging = false;
            return true;
            
        case SDL_MOUSEWHEEL:
            // Let views handle scrolling - don't return early, fall through to dispatch
            break;
        
        case SDL_TEXTINPUT:
            if (m_state.focusPathInput && (m_state.pathInputScreenVisible || m_state.showChangePathDialog)) {
                m_state.libraryPathInput += event.text.text;
                return true; // Consumed by path input
            }
            if (m_state.searchFocused) {
                 // Append to search query
                 // Note: ImGuiManager doesn't have direct access to filter logic, but 
                 // LibraryScreen reads m_state.searchQuery every frame.
                 m_state.searchQuery += event.text.text;
                 return true; // Consumed by search
            }
            break; // Fall through to view dispatch
            
        case SDL_KEYDOWN:
            // Input focus management
            if (!m_state.pathInputScreenVisible && !m_state.showChangePathDialog) {
                // Keep focusPathInput false if dialog closed
                // But don't force it false if search is focused?
                // Actually search focus is separate.
                m_state.focusPathInput = false;
            }
            
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
                return true; // Consumed by path input
            }
            
            if (m_state.searchFocused) {
                if (event.key.keysym.sym == SDLK_BACKSPACE && !m_state.searchQuery.empty()) {
                    m_state.searchQuery.pop_back();
                } else if (event.key.keysym.sym == SDLK_ESCAPE || event.key.keysym.sym == SDLK_RETURN) {
                    m_state.searchFocused = false;
                    SDL_StopTextInput();
                }
                return true; // Consumed by search
            }
            break; // Fall through to view dispatch
    }
    
    // Dispatch input to active view ONLY if no modals are active
    // Dispatch input to active view ONLY if no modals are active
    if (!m_state.scanDialogVisible && !m_state.showChangePathDialog && 
        !m_state.showUsbDialog && !m_state.showContextMenu &&
        !m_state.showAddToPlaylistDialog && !m_state.showPropertiesDialog &&
        !m_state.showRenamePlaylistDialog) 
    {
        auto it = m_views.find(m_state.currentTab);
        if (it != m_views.end() && it->second) {
            if (it->second->handleInput(event)) {
                 return true; // View consumed input
            }
        }
        return false; // No modal, view didn't handle -> return false (App handles shortcuts)
    }
    
    // Modals active -> Consumed all input
    return true;
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
    
    // Render registered view if available
    auto it = m_views.find(m_state.currentTab);
    if (it != m_views.end() && it->second) {
        it->second->render(*this);
    } else {
        // Fallback or specific handling for non-migrated tabs (e.g., Settings)
        if (m_state.currentTab == NavTab::Settings) {
             drawText("Settings", contentX + 20, contentY + 20, m_theme.textPrimary, 24);
             // ... (keep settings logic for now or TODO) ...
             // For brevity in this refactor, I'll just keep the existing Settings block or simplify it
             // actually I should keep the Settings block from the original file
             // Re-inserting the Settings block:
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
             if (m_state.currentTab != item.tab) {
                 // Hide old
                 auto itOld = m_views.find(m_state.currentTab);
                 if (itOld != m_views.end() && itOld->second) itOld->second->hide();
                 
                 m_state.currentTab = item.tab;
                 
                 // Show new
                 auto itNew = m_views.find(m_state.currentTab);
                 if (itNew != m_views.end() && itNew->second) itNew->second->show();
             }
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


// Helper for color unpacking


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
        if (menuH < 175) menuH = 175; // Increase height for extra options
        
        // Draw Menu Background
        drawRect(m_state.contextMenuX, m_state.contextMenuY, menuW, menuH, m_theme.surfaceHover);
        drawRect(m_state.contextMenuX, m_state.contextMenuY, menuW, menuH, m_theme.border, false); // Border
        
        // For unsupported files, only show Properties
        bool isSupported = !m_state.contextMediaItem.isUnsupported();

        // Item 1: Add to Playlist
        int y = m_state.contextMenuY;
        if (isSupported) {
            if (isMouseOver(m_state.contextMenuX, y, menuW, 35)) {
                drawRect(m_state.contextMenuX, y, menuW, 35, m_theme.primary);
                if (m_modalMouseClicked) {
                    m_state.showContextMenu = false;
                    m_state.showAddToPlaylistDialog = true;
                    m_modalMouseClicked = false;
                }
            }
            drawText("Add to Playlist", m_state.contextMenuX + 10, y + 8, m_theme.textPrimary, 14);
            y += 35;
        }

        // Item 2: Add to Queue
        if (isSupported) {
            if (isMouseOver(m_state.contextMenuX, y, menuW, 35)) {
                drawRect(m_state.contextMenuX, y, menuW, 35, m_theme.primary);
                if (m_modalMouseClicked) {
                     if (m_queueController) {
                        m_queueController->addToQueue(m_state.contextMediaItem);
                     }
                    m_state.showContextMenu = false;
                    m_modalMouseClicked = false;
                }
            }
            drawText("Add to Queue", m_state.contextMenuX + 10, y + 8, m_theme.textPrimary, 14);
            y += 35;
        }
        
        // Item 3: Play Next
        if (isSupported) {
            if (isMouseOver(m_state.contextMenuX, y, menuW, 35)) {
                drawRect(m_state.contextMenuX, y, menuW, 35, m_theme.primary);
                if (m_modalMouseClicked) {
                     if (m_queueController) {
                        m_queueController->addToQueueNext(m_state.contextMediaItem);
                     }
                    m_state.showContextMenu = false;
                    m_modalMouseClicked = false;
                }
            }
            drawText("Play Next", m_state.contextMenuX + 10, y + 8, m_theme.textPrimary, 14);
            y += 35;
        }
        
        // Item 4: Properties
        if (isMouseOver(m_state.contextMenuX, y, menuW, 35)) {
            drawRect(m_state.contextMenuX, y, menuW, 35, m_theme.primary);
            if (m_modalMouseClicked) {
                m_state.showContextMenu = false;
                m_state.showPropertiesDialog = true;
                const auto& media = m_state.contextMediaItem;
                m_state.metadataEdit.filePath = media.getFilePath();
                m_state.metadataEdit.fileName = media.getFileName();
                m_state.metadataEdit.extension = media.getExtension();
                m_state.metadataEdit.typeStr = media.isAudio() ? "Audio" : (media.isVideo() ? "Video" : (media.isUnsupported() ? "Unsupported" : "Unknown"));
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
                if (m_getMetadataForProperties && !media.isUnsupported()) {
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
                m_modalMouseClicked = false;
            }
        }
        drawText("Properties", m_state.contextMenuX + 10, y + 8, m_theme.textPrimary, 14);
        y += 35;

        // Item 5: Remove (Context Specific)
        if (m_state.contextMenuSource == ContextMenuSource::Queue) {
             if (isMouseOver(m_state.contextMenuX, y, menuW, 35)) {
                drawRect(m_state.contextMenuX, y, menuW, 35, m_theme.error); // Red for delete
                if (m_modalMouseClicked) {
                     if (m_queueController) {
                        m_queueController->removeByPath(m_state.contextMediaItem.getFilePath());
                     }
                    m_state.showContextMenu = false;
                    m_modalMouseClicked = false;
                }
            }
            drawText("Remove from Queue", m_state.contextMenuX + 10, y + 8, m_theme.textPrimary, 14);
        } else if (m_state.contextMenuSource == ContextMenuSource::Playlist) {
             // For Playlist, we need to know WHICH playlist.
             // ImGuiManager doesn't track "current context playlist ID" in UIState specifically, 
             // but PlaylistScreen usually sets m_selectedPlaylistId.
             // However, context menu is global. 
             // Let's rely on m_state.selectedPlaylistId if available.
             if (!m_state.selectedPlaylistId.empty()) {
                 if (isMouseOver(m_state.contextMenuX, y, menuW, 35)) {
                    drawRect(m_state.contextMenuX, y, menuW, 35, m_theme.error);
                    if (m_modalMouseClicked) {
                         if (m_playlistController) {
                            // Removing by Media Item is ambiguous if duplicates were allowed,
                            // but we just prevented duplicates in Queue.
                            // For Playlist, we might need index?
                            // m_playlistController->removeMediaFromPlaylist takes index. 
                            // Oof. We only have the MediaFileModel.
                            // Let's try to remove by ID/File. 
                            // PlaylistController needs a removeMediaByPath or similiar?
                            // Checked PlaylistController... only removeMediaFromPlaylist(id, index).
                            // We need to find the index of this item in the playlist.
                            // This is expensive but necessary here without refactoring context menu to pass index.
                            // Actually, m_state.selectedContextItemIndex WAS legacy but we can use it?
                            // Let's assume the view sets m_state.selectedContextItemIndex correctly!
                            // QueuePanel used to set it? 
                            // Let's check QueuePanel... it sets state.contextMediaItem.
                            // Let's update PlaylistScreen to set selectedContextItemIndex when opening context menu.
                            
                            // For now, let's assume we implement "removeMediaFromPlaylist(id, media)" or use index if available.
                            // Let's stick to using m_state.selectedContextItemIndex if >= 0.
                            if (m_state.selectedContextItemIndex >= 0) {
                                m_playlistController->removeMediaFromPlaylist(m_state.selectedPlaylistId, m_state.selectedContextItemIndex);
                            }
                         }
                        m_state.showContextMenu = false;
                        m_modalMouseClicked = false;
                    }
                }
                drawText("Remove from Playlist", m_state.contextMenuX + 10, y + 8, m_theme.textPrimary, 14);
             }
        }
        
        // Close if clicked outside
        if (m_modalMouseClicked && !isMouseOver(m_state.contextMenuX, m_state.contextMenuY, menuW, menuH)) {
            m_state.showContextMenu = false;
            m_modalMouseClicked = false;
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
                    if (m_modalMouseClicked) {
                        // Add to this playlist
                         m_playlistController->addMediaToPlaylist(pl.getId(), m_state.contextMediaItem);
                        m_state.showAddToPlaylistDialog = false;
                        m_modalMouseClicked = false;
                    }
                }
                drawText(pl.getName(), x + 30, listY + 6, m_theme.textPrimary, 14);
                listY += 35;
            }
        }
        
        // Cancel Button
        if (isMouseOver(x + dlgW - 100, y + dlgH - 40, 80, 30)) {
             drawRect(x + dlgW - 100, y + dlgH - 40, 80, 30, m_theme.surfaceActive);
             if (m_modalMouseClicked) {
                 m_state.showAddToPlaylistDialog = false;
                 m_modalMouseClicked = false;
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
             if (m_modalMouseClicked) {
                 m_state.showPropertiesDialog = false;
                 m_modalMouseClicked = false;
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
             if (m_modalMouseClicked) {
                 if (!m_state.renamePlaylistName.empty() && m_playlistController) {
                     m_playlistController->renamePlaylist(m_state.renamePlaylistId, m_state.renamePlaylistName);
                 }
                 m_state.showRenamePlaylistDialog = false;
                 SDL_StopTextInput();
                 m_modalMouseClicked = false;
             }
        }
        drawText("Save", x + dlgW - 160, y + dlgH - 33, m_theme.textPrimary, 14);
        
        // Cancel Button - always draw background
        bool cancelHover = isMouseOver(x + dlgW - 100, y + dlgH - 40, 70, 30);
        drawRect(x + dlgW - 100, y + dlgH - 40, 70, 30, cancelHover ? m_theme.surfaceActive : m_theme.surfaceHover);
        if (cancelHover && m_modalMouseClicked) {
            m_state.showRenamePlaylistDialog = false;
            m_state.renamePlaylistName.clear();
            SDL_StopTextInput();
            m_modalMouseClicked = false;
        }
        drawText("Cancel", x + dlgW - 90, y + dlgH - 33, m_theme.textPrimary, 14);
    }
}


} // namespace ui
} // namespace media_player

