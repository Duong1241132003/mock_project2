// Project includes
#include "views/QueuePanel.h"
#include "ui/ImGuiManager.h"
#include <iostream>

namespace media_player 
{
namespace views 
{

QueuePanel::QueuePanel(
    std::shared_ptr<controllers::QueueController> queueController,
    std::shared_ptr<controllers::PlaybackController> playbackController,
    std::shared_ptr<models::QueueModel> queueModel
)
    : m_queueController(queueController)
    , m_playbackController(playbackController)
    , m_queueModel(queueModel)
    , m_isVisible(false)
    , m_scrollOffset(0)
    , m_selectedIndex(-1)
{
}

QueuePanel::~QueuePanel() 
{
}

void QueuePanel::show() 
{
    m_isVisible = true;
}

void QueuePanel::hide() 
{
    m_isVisible = false;
}

void QueuePanel::update() 
{
    // Logic update if needed
}

bool QueuePanel::isVisible() const 
{
    return m_isVisible;
}

void QueuePanel::render(ui::ImGuiManager& painter)
{
    // Use layout constants from ImGuiManager
    int x = ui::ImGuiManager::GetSidebarWidth() + 10;
    int y = ui::ImGuiManager::GetMenuBarHeight() + 10;
    int w = painter.getWidth() - ui::ImGuiManager::GetSidebarWidth() - 20;
    
    const auto& theme = painter.getTheme();
    
    // Header
    std::string headerText = "Queue";
    if (m_queueController) {
        headerText += " (" + std::to_string(m_queueController->getQueueSize()) + " items)";
    }
    painter.drawText(headerText, x, y, theme.textPrimary, 20);
    
    // Clear Queue Button (if not empty)
    if (m_queueController && !m_queueController->isEmpty()) {
        int clearBtnW = 100;
        if (painter.isMouseOver(x + w - clearBtnW, y, clearBtnW, 25)) {
            painter.drawRect(x + w - clearBtnW, y, clearBtnW, 25, theme.surfaceHover);
            if (painter.isMouseClicked(x + w - clearBtnW, y, clearBtnW, 25)) {
                m_queueController->clearQueue();
                painter.consumeClick();
            }
        } else {
            painter.drawRect(x + w - clearBtnW, y, clearBtnW, 25, theme.surface);
        }
        painter.drawText("Clear Queue", x + w - clearBtnW + 10, y + 5, theme.textDim, 12);
    }

    y += 40;
    
    // Now Playing section
    painter.drawText("Now Playing", x, y, theme.textSecondary, 14);
    y += 25;
    
    painter.drawRect(x, y, w, 60, theme.surface);
    
    auto& state = painter.getState();
    if (!state.currentTrackTitle.empty()) {
        int npArtSize = 40;
        painter.drawRect(x + 10, y + 10, npArtSize, npArtSize, theme.surfaceHover);
        painter.drawText("~", x + 23, y + 20, theme.textDim, 20); // Placeholder art
        
        painter.drawText(state.currentTrackTitle, x + 60, y + 12, theme.primary, 16);
        painter.drawText(state.currentTrackArtist, x + 60, y + 35, theme.textSecondary, 12);
    } else {
        painter.drawText("No track playing", x + 20, y + 20, theme.textDim, 14);
    }
    y += 80;
    
    // Up Next section
    painter.drawText("Up Next", x, y, theme.textSecondary, 14);
    y += 25;
    
    if (!m_queueController || m_queueController->isEmpty()) {
        // Queue is empty message
        painter.drawRect(x, y, w, 100, theme.surface);
        painter.drawText("Queue is empty", x + 20, y + 40, theme.textDim, 14);
        painter.drawText("Play a song from Library to start", x + 20, y + 60, theme.textDim, 12);
    } else {
        // Queue List (playback order when shuffle on)
        auto queueItems = m_queueController->getPlaybackOrderItems();
        size_t currentLogicalIndex = m_queueController->getCurrentIndex();
        int listH = painter.getHeight() - y - ui::ImGuiManager::GetPlayerBarHeight() - 20;
        int itemHeight = 50; 
        
        // Clip rect for list
        SDL_Rect clipRect = { x, y, w, listH };
        SDL_RenderSetClipRect(painter.getRenderer(), &clipRect);
        
        for (size_t i = 0; i < queueItems.size(); i++) {
            const auto& media = queueItems[i];
            
            // Calculate Y with scroll offset
            int itemY = y + (i * itemHeight) - m_scrollOffset;
            
            // Culling
            if (itemY + itemHeight < y || itemY > y + listH) continue;
            
            bool hover = painter.isMouseOver(x, itemY, w, itemHeight);
            // Check if mouse inside List Area
            bool mouseInList = painter.isMouseOver(x, y, w, listH);
            hover = hover && mouseInList;
            
            painter.drawRect(x, itemY, w, itemHeight, hover ? theme.surfaceHover : (i % 2 == 0 ? theme.background : theme.surface));
            
            // Context Menu (Right Click)
            if (hover && mouseInList && painter.isRightMouseClicked(x, itemY, w, itemHeight)) {
                 int mx, my;
                 SDL_GetMouseState(&mx, &my);
                 state.showContextMenu = true;
                 state.contextMenuX = mx;
                 state.contextMenuY = my;
                 state.contextMediaItem = media;
                 state.contextMenuSource = ui::ContextMenuSource::Queue;
                 painter.consumeClick();
            }
            
            // Index (position in playback order)
            bool isCurrent = (state.isPlaying && static_cast<size_t>(i) == currentLogicalIndex);
            std::string marker = isCurrent ? "> " : "";
            painter.drawText(marker + std::to_string(i + 1), x + 10, itemY + 15, theme.textDim, 12);
            
            // Title
            std::string title = media.getTitle().empty() ? media.getFileName() : media.getTitle();
            if (title.length() > 50) title = title.substr(0, 47) + "...";
            uint32_t titleColor = isCurrent ? theme.success : theme.textPrimary;
            painter.drawText(title, x + 40, itemY + 15, titleColor, 14);
            
            // Artist
            std::string artist = media.getArtist();
            if (artist.length() > 30) artist = artist.substr(0, 27) + "...";
            if (!artist.empty()) {
                painter.drawText(artist, x + w / 2, itemY + 15, theme.textSecondary, 12);
            }
            
            // Click to jump (Left Click)
            if (hover && mouseInList && painter.isLeftMouseClicked(x, itemY, w, itemHeight)) {
                 if (m_playbackController) {
                     std::cout << "[QueuePanel] Clicked item " << i << " (" << media.getTitle() << ")" << std::endl;
                     bool result = m_playbackController->playItemAt(i);
                     std::cout << "[QueuePanel] playItemAt(" << i << ") returned " << (result ? "true" : "false") << std::endl;
                 }
                 painter.consumeClick();
            }
        }
        SDL_RenderSetClipRect(painter.getRenderer(), nullptr);
    }
}

bool QueuePanel::handleInput(const SDL_Event& event) 
{
    if (event.type == SDL_MOUSEWHEEL) {
         m_scrollOffset -= event.wheel.y * 30;
         if (m_scrollOffset < 0) m_scrollOffset = 0;
         return true;
    }
    return false;
}

void QueuePanel::removeSelectedItem() 
{
    size_t currentIndex = m_queueModel->getCurrentIndex();
    m_queueController->removeFromQueue(currentIndex);
}

void QueuePanel::clearQueue() 
{
    m_queueController->clearQueue();
}

void QueuePanel::moveItemUp(size_t index) 
{
    if (index > 0) 
    {
        m_queueController->moveItem(index, index - 1);
    }
}

void QueuePanel::moveItemDown(size_t index) 
{
    if (index < m_queueModel->size() - 1) 
    {
        m_queueController->moveItem(index, index + 1);
    }
}

void QueuePanel::jumpToItem(size_t index) 
{
    if (index < m_queueModel->size()) 
    {
        // TODO: Jump to this item in queue
    }
}

void QueuePanel::toggleShuffle() 
{
    m_queueController->toggleShuffle();
}

void QueuePanel::toggleRepeat() 
{
    m_queueController->toggleRepeat();
}

void QueuePanel::displayQueue() 
{
    // Replaced by render()
}

} // namespace views
} // namespace media_player
