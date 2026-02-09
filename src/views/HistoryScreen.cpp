// Project includes
#include "views/HistoryScreen.h"
#include "ui/ImGuiManager.h"
#include <filesystem>

namespace media_player 
{
namespace views 
{

HistoryScreen::HistoryScreen(
    std::shared_ptr<repositories::HistoryRepository> historyRepo,
    std::shared_ptr<controllers::QueueController> queueController,
    std::shared_ptr<controllers::PlaybackController> playbackController
)
    : m_historyRepo(historyRepo)
    , m_queueController(queueController)
    , m_playbackController(playbackController)
    , m_isVisible(false)
    , m_scrollOffset(0)
{
}

HistoryScreen::~HistoryScreen() 
{
}

void HistoryScreen::show() 
{
    m_isVisible = true;
    refreshHistory();
}

void HistoryScreen::hide() 
{
    m_isVisible = false;
}

void HistoryScreen::update() 
{
    // Auto-refresh logic could go here if needed
}

bool HistoryScreen::isVisible() const 
{
    return m_isVisible;
}

void HistoryScreen::refreshHistory()
{
    if (m_historyRepo) {
        m_historyList = m_historyRepo->getAllHistory();
        // Sort by timestamp desc? Repository might already do it.
    }
}

void HistoryScreen::render(ui::ImGuiManager& painter)
{
    // Auto-refresh history để UI luôn cập nhật (fix cho playPrevious)
    refreshHistory();
    
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
    if (state.currentTrackTitle.empty() && m_historyList.empty()) {
        painter.drawRect(x, y, w, 100, theme.surface);
        painter.drawText("No playback history yet", x + 20, y + 40, theme.textDim, 14);
        painter.drawText("Play some tracks to see them here", x + 20, y + 60, theme.textDim, 12);
        return;
    } 
    
    if (!state.currentTrackTitle.empty()) {
        // Show current/last played track
        painter.drawRect(x, y, w, 60, theme.surface);
        painter.drawText(">", x + 15, y + 18, theme.success, 18);
        painter.drawText(state.currentTrackTitle, x + 50, y + 12, theme.textPrimary, 16);
        painter.drawText(state.currentTrackArtist, x + 50, y + 35, theme.textSecondary, 12);
        painter.drawText("Now Playing", x + w - 100, y + 20, theme.success, 12);
        y += 70;
    }
    
    // History List
    if (!m_historyList.empty()) {
        int listAreaH = painter.getHeight() - ui::ImGuiManager::GetMenuBarHeight() - ui::ImGuiManager::GetPlayerBarHeight() - y - 20;
        int itemHeight = 50;
        
        // Clip rect
        SDL_Rect listClip = {x, y, w, listAreaH};
        SDL_RenderSetClipRect(painter.getRenderer(), &listClip);
        
        int index = 0;
        for (const auto& entry : m_historyList) {
            int itemY = y + (index * itemHeight) - m_scrollOffset;
             
             // Culling
            if (itemY + itemHeight < y || itemY > y + listAreaH) {
                index++;
                continue;
            }
            
            bool mouseInList = painter.isMouseOver(x, y, w, listAreaH);
            bool hover = mouseInList && painter.isMouseOver(x, itemY, w, itemHeight);
             
             // Alternating background
            uint32_t bg = (index % 2 == 0) ? theme.background : theme.surface;
            if (hover) bg = theme.surfaceHover;
            
            painter.drawRect(x, itemY, w, itemHeight, bg);
            
            // Serial number (1-based)
            painter.drawText(std::to_string(index + 1), x + 15, itemY + 15, theme.textDim, 14);
            
            bool fileExists = std::filesystem::exists(entry.media.getFilePath());
            std::string title = entry.media.getTitle().empty() ? entry.media.getFileName() : entry.media.getTitle();
            if (!fileExists) title += " (file not found)";
            if (title.length() > 50) title = title.substr(0, 47) + "...";
            uint32_t titleColor = fileExists ? theme.textPrimary : theme.textDim;
            painter.drawText(title, x + 50, itemY + 15, titleColor, 14);
            
            std::string artist = entry.media.getArtist();
            if (!artist.empty()) painter.drawText(artist, x + w / 2, itemY + 15, fileExists ? theme.textSecondary : theme.textDim, 14);
            
            // Click to Play (Left Click only)
             if (hover && painter.isMouseClicked(x, itemY, w, itemHeight) && 
                 !state.showContextMenu && 
                 m_queueController && m_playbackController) {
                     
                m_queueController->addToQueue(entry.media);
                // Play the newly added item (at the end)
                if (m_queueController->getQueueSize() > 0) {
                     size_t newIndex = m_queueController->getQueueSize() - 1;
                     m_playbackController->playItemAt(newIndex);
                }
                 painter.consumeClick();
             }

            index++;
        }
        
        SDL_RenderSetClipRect(painter.getRenderer(), nullptr);
    } else {
        painter.drawText("No history available.", x, y + 20, theme.textDim, 14);
    }
}

bool HistoryScreen::handleInput(const SDL_Event& event) 
{
     if (event.type == SDL_MOUSEWHEEL) {
         m_scrollOffset -= event.wheel.y * 30;
         if (m_scrollOffset < 0) m_scrollOffset = 0;
         return true;
    }
    return false;
}

} // namespace views
} // namespace media_player
