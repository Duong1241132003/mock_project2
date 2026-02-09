// Project includes
#include "views/PlaylistScreen.h"
#include "ui/ImGuiManager.h"

namespace media_player 
{
namespace views 
{

PlaylistScreen::PlaylistScreen(
    std::shared_ptr<controllers::PlaylistController> playlistController,
    std::shared_ptr<controllers::PlaybackController> playbackController,
    std::shared_ptr<controllers::QueueController> queueController
)
    : m_playlistController(playlistController)
    , m_playbackController(playbackController)
    , m_queueController(queueController)
    , m_isVisible(false)
    , m_showCreateDialog(false)
    , m_showRenameDialog(false)
{
}

PlaylistScreen::~PlaylistScreen() 
{
}

void PlaylistScreen::show() 
{
    m_isVisible = true;
    refreshPlaylistList();
}

void PlaylistScreen::hide() 
{
    m_isVisible = false;
    m_showCreateDialog = false;
    m_showRenameDialog = false;
    SDL_StopTextInput();
}

void PlaylistScreen::update() 
{
    // Logic update if needed
}

bool PlaylistScreen::isVisible() const 
{
    return m_isVisible;
}

void PlaylistScreen::render(ui::ImGuiManager& painter)
{
    // Use layout constants from ImGuiManager
    int x = ui::ImGuiManager::GetSidebarWidth() + 10;
    int y = ui::ImGuiManager::GetMenuBarHeight() + 10;
    int w = painter.getWidth() - ui::ImGuiManager::GetSidebarWidth() - 20;
    int h = painter.getHeight() - ui::ImGuiManager::GetMenuBarHeight() - ui::ImGuiManager::GetPlayerBarHeight();

    const auto& theme = painter.getTheme();

    // Header
    painter.drawText("Playlists", x, y, theme.textPrimary, 20);
    y += 40;
    
    // New Playlist button
    int btnW = 150;
    int btnH = 35;
    bool btnHover = painter.isMouseOver(x, y, btnW, btnH);
    painter.drawRect(x, y, btnW, btnH, btnHover ? theme.primaryHover : theme.primary);
    painter.drawText("+ New Playlist", x + 15, y + 9, theme.textPrimary, 14);
    
    // Handle "New Playlist" button click
    if (btnHover && painter.isMouseClicked(x, y, btnW, btnH)) {
         m_showCreateDialog = true;
         m_newPlaylistName.clear();
         SDL_StartTextInput();
         painter.consumeClick();
    }
    
    y += 50;
    
    // Create Playlist Dialog Overlay
    if (m_showCreateDialog) {
        int dlgW = 300;
        int dlgH = 150;
        int dlgX = x + w/2 - 150;
        int dlgY = y; 
        
        painter.drawRect(dlgX, dlgY, dlgW, dlgH, theme.surface);
        painter.drawRect(dlgX, dlgY, dlgW, dlgH, theme.border, false); // Border
        
        painter.drawText("Create New Playlist", dlgX + 20, dlgY + 20, theme.textPrimary, 16);
        
        // Input box
        painter.drawRect(dlgX + 20, dlgY + 50, 260, 30, theme.background);
        painter.drawText(m_newPlaylistName.empty() ? "Enter name..." : m_newPlaylistName, 
                 dlgX + 30, dlgY + 58, theme.textPrimary, 14);
        
        // Cursor blink
        if (SDL_GetTicks() % 1000 < 500) {
            int textW = m_newPlaylistName.length() * 8; // Approx width
            painter.drawRect(dlgX + 30 + textW, dlgY + 55, 2, 20, theme.textPrimary);
        }
        
        // Buttons
        // Create
        bool createHover = painter.isMouseOver(dlgX + 170, dlgY + 100, 100, 30);
        painter.drawRect(dlgX + 170, dlgY + 100, 100, 30, createHover ? theme.primaryHover : theme.primary);
        painter.drawText("Create", dlgX + 195, dlgY + 107, theme.textPrimary, 14);
        
        if (createHover && painter.isMouseClicked(dlgX + 170, dlgY + 100, 100, 30)) {
            if (!m_newPlaylistName.empty() && m_playlistController) {
                m_playlistController->createPlaylist(m_newPlaylistName);
                m_showCreateDialog = false;
                m_newPlaylistName.clear();
                SDL_StopTextInput();
                refreshPlaylistList();
            }
            painter.consumeClick();
        }
        
        // Cancel
        bool cancelHover = painter.isMouseOver(dlgX + 30, dlgY + 100, 100, 30);
        painter.drawRect(dlgX + 30, dlgY + 100, 100, 30, cancelHover ? theme.surfaceHover : theme.surfaceActive);
        painter.drawText("Cancel", dlgX + 55, dlgY + 107, theme.textPrimary, 14);
        
        if (cancelHover && painter.isMouseClicked(dlgX + 30, dlgY + 100, 100, 30)) {
            m_showCreateDialog = false;
            m_newPlaylistName.clear();
            SDL_StopTextInput();
            painter.consumeClick();
        }
        
        return; // Modal blocks underlying UI
    }
    
    // Rename Playlist Dialog Overlay
    if (m_showRenameDialog) {
        int dlgW = 300;
        int dlgH = 150;
        int dlgX = x + w/2 - 150;
        int dlgY = y; 
        
        painter.drawRect(dlgX, dlgY, dlgW, dlgH, theme.surface);
        painter.drawRect(dlgX, dlgY, dlgW, dlgH, theme.border, false); // Border
        
        painter.drawText("Rename Playlist", dlgX + 20, dlgY + 20, theme.textPrimary, 16);
        
        // Input box
        painter.drawRect(dlgX + 20, dlgY + 50, 260, 30, theme.background);
        painter.drawText(m_renamePlaylistName.empty() ? "" : m_renamePlaylistName, 
                 dlgX + 30, dlgY + 58, theme.textPrimary, 14);
                 
         // Cursor blink
        if (SDL_GetTicks() % 1000 < 500) {
            int textW = m_renamePlaylistName.length() * 8; 
            painter.drawRect(dlgX + 30 + textW, dlgY + 55, 2, 20, theme.textPrimary);
        }
        
        // Buttons
        // Save
        bool saveHover = painter.isMouseOver(dlgX + 170, dlgY + 100, 100, 30);
        painter.drawRect(dlgX + 170, dlgY + 100, 100, 30, saveHover ? theme.primaryHover : theme.primary);
        painter.drawText("Save", dlgX + 200, dlgY + 107, theme.textPrimary, 14);
        
        if (saveHover && painter.isMouseClicked(dlgX + 170, dlgY + 100, 100, 30)) {
            if (!m_renamePlaylistName.empty() && m_playlistController) {
                m_playlistController->renamePlaylist(m_renamePlaylistId, m_renamePlaylistName);
                m_showRenameDialog = false;
                SDL_StopTextInput();
                refreshPlaylistList();
            }
            painter.consumeClick();
        }
        
        // Cancel
        bool cancelHover = painter.isMouseOver(dlgX + 30, dlgY + 100, 100, 30);
        painter.drawRect(dlgX + 30, dlgY + 100, 100, 30, cancelHover ? theme.surfaceHover : theme.surfaceActive);
        painter.drawText("Cancel", dlgX + 55, dlgY + 107, theme.textPrimary, 14);
        
        if (cancelHover && painter.isMouseClicked(dlgX + 30, dlgY + 100, 100, 30)) {
            m_showRenameDialog = false;
            SDL_StopTextInput();
            painter.consumeClick();
        }
        
        return; // Modal
    }

    if (m_playlistController) {
        // If a playlist is selected, show its details
        if (!m_selectedPlaylistId.empty()) {
            auto playlistOpt = m_playlistController->getPlaylistById(m_selectedPlaylistId);
            if (!playlistOpt) {
                m_selectedPlaylistId.clear(); // Playlist deleted or invalid
                return;
            }
            const auto& playlist = *playlistOpt;
            
            // Back Button
            if (painter.isMouseOver(x, y, 60, 30)) {
                painter.drawRect(x, y, 60, 30, theme.surfaceHover);
                if (painter.isMouseClicked(x, y, 60, 30)) {
                    m_selectedPlaylistId.clear();
                    painter.consumeClick();
                    return;
                }
            }
            painter.drawText("< Back", x + 10, y + 8, theme.textPrimary, 14);
            
            // Playlist Name
            painter.drawText(playlist.getName(), x + 80, y + 5, theme.textPrimary, 20);
            
            // Rename Button
            int renameX = x + w - 170;
            if (painter.isMouseOver(renameX, y, 80, 30)) {
                painter.drawRect(renameX, y, 80, 30, theme.surfaceHover);
                if (painter.isMouseClicked(renameX, y, 80, 30)) {
                    m_showRenameDialog = true;
                    m_renamePlaylistId = playlist.getId();
                    m_renamePlaylistName = playlist.getName();
                    SDL_StartTextInput();
                    painter.consumeClick();
                }
            }
            painter.drawText("Rename", renameX + 15, y + 8, theme.textPrimary, 12);
            
             // Delete Button
            int deleteX = x + w - 80;
             if (painter.isMouseOver(deleteX, y, 80, 30)) {
                painter.drawRect(deleteX, y, 80, 30, theme.error);
                if (painter.isMouseClicked(deleteX, y, 80, 30)) {
                    m_playlistController->deletePlaylist(playlist.getId());
                    m_selectedPlaylistId.clear();
                    refreshPlaylistList();
                    painter.consumeClick();
                    return;
                }
            }
            painter.drawText("Delete", deleteX + 18, y + 8, theme.textPrimary, 12);

            y += 40;
            
            // List Items
            const auto& items = playlist.getItems();
            if (items.empty()) {
                painter.drawText("Playlist is empty. Add songs from Library (Right-click).", x, y + 20, theme.textDim, 14);
            }
            
            int itemY = y;
            for (size_t i = 0; i < items.size(); ++i) {
                if (itemY > h + ui::ImGuiManager::GetMenuBarHeight()) break; // Clip
                
                const auto& media = items[i];
                bool hover = painter.isMouseOver(x, itemY, w, 40);
                
                if (hover) painter.drawRect(x, itemY, w, 40, theme.surfaceHover); 
                else if (i % 2 == 0) painter.drawRect(x, itemY, w, 40, theme.background);
                
                // Play on click
                if (hover && painter.isLeftMouseClicked(x, itemY, w, 40)) {
                    if (m_playbackController && m_queueController) {
                         // Replace queue with playlist content
                         // Disable shuffle to ensure the playlist order is preserved
                         if (m_queueController->isShuffleEnabled()) {
                             m_queueController->setShuffle(false);
                         }
                         
                         m_queueController->clearQueue();
                         m_queueController->addMultipleToQueue(items);
                         // Play the selected item by index
                         m_playbackController->playItemAt(i);
                    }
                    painter.consumeClick();
                }

                painter.drawText(media.getTitle().empty() ? media.getFileName() : media.getTitle(), x + 10, itemY + 10, theme.textPrimary, 14);
                painter.drawText(media.getArtist(), x + w/2, itemY + 10, theme.textSecondary, 14);

                // Context Menu (Right Click)
                if (hover && painter.isRightMouseClicked(x, itemY, w, 40)) {
                     int mx, my;
                     SDL_GetMouseState(&mx, &my);
                     auto& state = painter.getState();
                     state.showContextMenu = true;
                     state.contextMenuX = mx;
                     state.contextMenuY = my;
                     state.contextMediaItem = media;
                     state.contextMenuSource = ui::ContextMenuSource::Playlist;
                     state.selectedContextItemIndex = static_cast<int>(i); // Set index for removal
                     state.selectedPlaylistId = m_selectedPlaylistId; // Set playlist ID for ImGuiManager
                     painter.consumeClick();
                }
                
                // Remove button for item?
                int removeBtnX = x + w - 40;
                if (painter.isMouseOver(removeBtnX, itemY, 30, 30)) {
                     painter.drawText("x", removeBtnX + 10, itemY + 8, theme.error, 14);
                     if (painter.isMouseClicked(removeBtnX, itemY, 30, 30)) {
                         m_playlistController->removeMediaFromPlaylist(playlist.getId(), i);
                         refreshPlaylistList(); // Actually we need to refresh items? getPlaylistById returns copy?
                         // m_playlistController->getPlaylistById returns optional<PlaylistModel>. 
                         // Check if we need to re-fetch. Yes.
                         // But we are inside loop.
                         painter.consumeClick();
                         return; // Break frame to avoid crash
                     }
                }
                
                itemY += 40;
            }
        
        } else {
            // List All Playlists
            if (m_playlists.empty()) {
                painter.drawText("No playlists created yet.", x, y + 20, theme.textDim, 14);
            }
            
            int pY = y;
            for (const auto& pl : m_playlists) {
                if (pY > h + ui::ImGuiManager::GetMenuBarHeight()) break;
                
                bool hover = painter.isMouseOver(x, pY, w, 50);
                painter.drawRect(x, pY, w, 50, hover ? theme.surfaceHover : theme.surface);
                painter.drawRect(x, pY, w, 50, theme.border, false);
                
                painter.drawIcon("[P]", x + 15, pY + 15, theme.primary, 20);
                painter.drawText(pl.getName(), x + 50, pY + 15, theme.textPrimary, 16);
                std::string count = std::to_string(pl.getItemCount()) + " items";
                painter.drawText(count, x + w - 100, pY + 18, theme.textSecondary, 12);
                
                if (hover && painter.isMouseClicked(x, pY, w, 50)) {
                    m_selectedPlaylistId = pl.getId();
                    painter.consumeClick();
                }
                
                pY += 55;
            }
        }
    }
}

bool PlaylistScreen::handleInput(const SDL_Event& event) 
{
    if (m_showCreateDialog) {
        if (event.type == SDL_TEXTINPUT) {
            m_newPlaylistName += event.text.text;
            return true;
        } else if (event.type == SDL_KEYDOWN) {
            if (event.key.keysym.sym == SDLK_BACKSPACE && !m_newPlaylistName.empty()) {
                m_newPlaylistName.pop_back();
            } else if (event.key.keysym.sym == SDLK_ESCAPE) {
                m_showCreateDialog = false;
                SDL_StopTextInput();
            } else if (event.key.keysym.sym == SDLK_RETURN) {
                 if (!m_newPlaylistName.empty() && m_playlistController) {
                    m_playlistController->createPlaylist(m_newPlaylistName);
                    m_showCreateDialog = false;
                    m_newPlaylistName.clear();
                    SDL_StopTextInput();
                    refreshPlaylistList();
                }
            }
            return true;
        }
    }
    
    if (m_showRenameDialog) {
        if (event.type == SDL_TEXTINPUT) {
            m_renamePlaylistName += event.text.text;
            return true;
        } else if (event.type == SDL_KEYDOWN) {
             if (event.key.keysym.sym == SDLK_BACKSPACE && !m_renamePlaylistName.empty()) {
                m_renamePlaylistName.pop_back();
            } else if (event.key.keysym.sym == SDLK_ESCAPE) {
                m_showRenameDialog = false;
                SDL_StopTextInput();
            } else if (event.key.keysym.sym == SDLK_RETURN) {
                if (!m_renamePlaylistName.empty() && m_playlistController) {
                    m_playlistController->renamePlaylist(m_renamePlaylistId, m_renamePlaylistName);
                    m_showRenameDialog = false;
                    SDL_StopTextInput();
                    refreshPlaylistList();
                }
            }
            return true;
        }
    }
    
    return false;
}

void PlaylistScreen::refreshPlaylistList() 
{
    if (m_playlistController) {
        m_playlists = m_playlistController->getAllPlaylists();
    }
}

} // namespace views
} // namespace media_player
