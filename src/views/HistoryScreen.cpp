// Project includes
#include "views/HistoryScreen.h"
#include "ui/ImGuiManager.h"

// System includes
#include <filesystem>

namespace media_player 
{
namespace views 
{

// ============================================================================
// Constructor & Destructor
// ============================================================================

HistoryScreen::HistoryScreen(
    std::shared_ptr<controllers::HistoryController> historyController
)
    : m_historyController(historyController)
    , m_isVisible(false)
    , m_scrollOffset(0)
{
}

HistoryScreen::~HistoryScreen() 
{
}

// ============================================================================
// IView Interface
// ============================================================================

void HistoryScreen::show() 
{
    m_isVisible = true;
    refreshCache();
}

void HistoryScreen::hide() 
{
    m_isVisible = false;
}

void HistoryScreen::update() 
{
    // Auto-refresh có thể được thêm ở đây nếu cần
}

bool HistoryScreen::isVisible() const 
{
    return m_isVisible;
}

// ============================================================================
// Private Methods
// ============================================================================

void HistoryScreen::refreshCache() 
{
    if (m_historyController) 
    {
        m_cachedHistory = m_historyController->getHistoryEntries();
    }
}

// ============================================================================
// Rendering
// ============================================================================

void HistoryScreen::render(ui::ImGuiManager& painter)
{
    // Auto-refresh history để UI luôn cập nhật (fix cho playPrevious)
    refreshCache();
    
    // Use layout constants from ImGuiManager
    int x = ui::ImGuiManager::GetSidebarWidth() + 10;
    int y = ui::ImGuiManager::GetMenuBarHeight() + 10;
    int w = painter.getWidth() - ui::ImGuiManager::GetSidebarWidth() - 20;
    
    const auto& theme = painter.getTheme();
    
    // Header
    painter.drawText("History", x, y, theme.textPrimary, 20);
    y += 40;
    
    // Recently Played section
    painter.drawText("Recently Played", x, y, theme.textSecondary, 14);
    y += 25;
    
    auto& state = painter.getState();
    
    // Check if we have history or current track
    if (state.currentTrackTitle.empty() && m_cachedHistory.empty()) 
    {
        painter.drawRect(x, y, w, 100, theme.surface);
        painter.drawText("No playback history yet", x + 20, y + 40, theme.textDim, 14);
        painter.drawText("Play some tracks to see them here", x + 20, y + 60, theme.textDim, 12);
        return;
    } 
    
    if (!state.currentTrackTitle.empty()) 
    {
        // Show current/last played track
        painter.drawRect(x, y, w, 60, theme.surface);
        painter.drawText(">", x + 15, y + 18, theme.success, 18);
        painter.drawText(state.currentTrackTitle, x + 50, y + 12, theme.textPrimary, 16);
        painter.drawText(state.currentTrackArtist, x + 50, y + 35, theme.textSecondary, 12);
        painter.drawText("Now Playing", x + w - 100, y + 20, theme.success, 12);
        y += 70;
    }
    
    // History List
    if (!m_cachedHistory.empty()) 
    {
        int listAreaH = painter.getHeight() - ui::ImGuiManager::GetMenuBarHeight() 
                        - ui::ImGuiManager::GetPlayerBarHeight() - y - 20;
        int itemHeight = 50;
        
        // Clip rect
        SDL_Rect listClip = {x, y, w, listAreaH};
        SDL_RenderSetClipRect(painter.getRenderer(), &listClip);
        
        int index = 0;
        for (const auto& entry : m_cachedHistory) 
        {
            int itemY = y + (index * itemHeight) - m_scrollOffset;
             
            // Culling - skip items outside visible area
            if (itemY + itemHeight < y || itemY > y + listAreaH) 
            {
                index++;
                continue;
            }
            
            bool mouseInList = painter.isMouseOver(x, y, w, listAreaH);
            bool hover = mouseInList && painter.isMouseOver(x, itemY, w, itemHeight);
             
            // Alternating background
            uint32_t bg = (index % 2 == 0) ? theme.background : theme.surface;
            if (hover) 
            {
                bg = theme.surfaceHover;
            }
            
            painter.drawRect(x, itemY, w, itemHeight, bg);
            
            // Serial number (1-based)
            painter.drawText(std::to_string(index + 1), x + 15, itemY + 15, theme.textDim, 14);
            
            // Check file existence
            bool fileExists = std::filesystem::exists(entry.media.getFilePath());
            std::string title = entry.media.getTitle().empty() 
                ? entry.media.getFileName() 
                : entry.media.getTitle();
            
            if (!fileExists) 
            {
                title += " (file not found)";
            }
            
            if (title.length() > 50) 
            {
                title = title.substr(0, 47) + "...";
            }
            
            uint32_t titleColor = fileExists ? theme.textPrimary : theme.textDim;
            painter.drawText(title, x + 50, itemY + 15, titleColor, 14);
            
            // Artist info
            std::string artist = entry.media.getArtist();
            if (!artist.empty()) 
            {
                uint32_t artistColor = fileExists ? theme.textSecondary : theme.textDim;
                painter.drawText(artist, x + w / 2, itemY + 15, artistColor, 14);
            }
            
            // Click to Play (Left Click only) - delegate to controller
            if (hover && painter.isMouseClicked(x, itemY, w, itemHeight) && 
                !state.showContextMenu && m_historyController) 
            {
                m_historyController->playFromHistory(static_cast<size_t>(index));
                painter.consumeClick();
            }

            index++;
        }
        
        SDL_RenderSetClipRect(painter.getRenderer(), nullptr);
    } 
    else 
    {
        painter.drawText("No history available.", x, y + 20, theme.textDim, 14);
    }
}

bool HistoryScreen::handleInput(const SDL_Event& event) 
{
    if (event.type == SDL_MOUSEWHEEL) 
    {
        m_scrollOffset -= event.wheel.y * 30;
        if (m_scrollOffset < 0) 
        {
            m_scrollOffset = 0;
        }
        return true;
    }
    return false;
}

} // namespace views
} // namespace media_player
