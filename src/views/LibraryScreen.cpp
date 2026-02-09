// Project includes
#include "views/LibraryScreen.h"
#include "config/AppConfig.h"
#include "ui/ImGuiManager.h" // Include for rendering primitives

#include <algorithm>
#include <iomanip>
#include <sstream>

namespace media_player 
{
namespace views 
{

LibraryScreen::LibraryScreen(
    std::shared_ptr<controllers::LibraryController> libraryController,
    std::shared_ptr<controllers::QueueController> queueController,
    std::shared_ptr<controllers::PlaybackController> playbackController,
    std::shared_ptr<controllers::PlaylistController> playlistController
)
    : m_libraryController(libraryController)
    , m_queueController(queueController)
    , m_playbackController(playbackController)
    , m_playlistController(playlistController)
    , m_currentPage(0)
    , m_selectedIndex(-1)
    , m_scrollOffset(0)
    , m_sortField(0)
    , m_sortAscending(true)
    , m_searchFilter(0)
    , m_isVisible(false)
    , m_showContextMenu(false)
    , m_contextMenuX(0)
    , m_contextMenuY(0)
    , m_contextMenuIndex(-1)
{
}

LibraryScreen::~LibraryScreen() 
{
}

void LibraryScreen::show() 
{
    m_isVisible = true;
    refreshMediaList();
}

void LibraryScreen::hide() 
{
    m_isVisible = false;
}

void LibraryScreen::update() 
{
    // Logic update if needed
}

bool LibraryScreen::isVisible() const 
{
    return m_isVisible;
}

void LibraryScreen::render(ui::ImGuiManager& painter)
{
    // Use correct getters for layout
    int x = ui::ImGuiManager::GetSidebarWidth() + 10;
    int y = ui::ImGuiManager::GetMenuBarHeight() + 10;
    int w = painter.getWidth() - ui::ImGuiManager::GetSidebarWidth() - 20;
    int h = painter.getHeight() - ui::ImGuiManager::GetMenuBarHeight() - ui::ImGuiManager::GetPlayerBarHeight() - 20;
    
    const auto& theme = painter.getTheme();

    // Sync Search Query from Global State (handled by ImGuiManager)
    if (m_searchQuery != painter.getState().searchQuery) {
        m_searchQuery = painter.getState().searchQuery;
    }
    
    // 1. Filter List (Search & Sort)
    // Only re-filter if list is empty or search changed? 
    // Actually best to keep a filtered list or filter on demand.
    // For now, let's filter on every frame or cache it? caching is better but `refreshMediaList` handles loading.
    // Let's assume m_currentMediaList IS the filtered list for simplicity, updated when search changes.
    // But m_currentMediaList is populated by `getAllMedia` initially.
    // We should probably filter in-place or separate source vs display list.
    // For now, let's use a local filtered vector like ImGuiManager did, BUT we should optimize this later.
    // Refactoring: ImGuiManager logic was doing filtering every frame. Let's replicate that logic but maybe cleaner.
    
    // Actually, `m_currentMediaList` IS the source. We should define what to render.
    // Let's just use m_currentMediaList as the SOURCE and create indices.
    std::vector<size_t> filteredIndices;
    filteredIndices.reserve(m_currentMediaList.size());
    
    for (size_t i = 0; i < m_currentMediaList.size(); i++) {
        const auto& media = m_currentMediaList[i];
        
        if (m_searchQuery.empty()) {
            filteredIndices.push_back(i);
        } else {
             std::string query = m_searchQuery;
             std::transform(query.begin(), query.end(), query.begin(), ::tolower);
             
             std::string title = media.getTitle().empty() ? media.getFileName() : media.getTitle();
             std::transform(title.begin(), title.end(), title.begin(), ::tolower);
             
             std::string artist = media.getArtist();
             std::transform(artist.begin(), artist.end(), artist.begin(), ::tolower);
             
             std::string album = media.getAlbum();
             std::transform(album.begin(), album.end(), album.begin(), ::tolower);
             
             bool matches = false;
             // 0=All, 1=Title, 2=Artist, 3=Album
             switch (m_searchFilter) {
                 case 0: // All
                     matches = (title.find(query) != std::string::npos || 
                                artist.find(query) != std::string::npos ||
                                album.find(query) != std::string::npos);
                     break;
                 case 1: // Title
                     matches = (title.find(query) != std::string::npos);
                     break;
                 case 2: // Artist
                     matches = (artist.find(query) != std::string::npos);
                     break;
                 case 3: // Album
                     matches = (album.find(query) != std::string::npos);
                     break;
             }
             if (matches) filteredIndices.push_back(i);
        }
    }

    // Sort Indices
    if (!filteredIndices.empty()) {
         std::stable_sort(filteredIndices.begin(), filteredIndices.end(), 
            [&](size_t a, size_t b) {
                const auto& mA = m_currentMediaList[a];
                const auto& mB = m_currentMediaList[b];
                int cmp = 0;
                if (m_sortField == 0) { // Title
                    std::string tA = mA.getTitle().empty() ? mA.getFileName() : mA.getTitle();
                    std::string tB = mB.getTitle().empty() ? mB.getFileName() : mB.getTitle();
                    cmp = tA.compare(tB);
                } else if (m_sortField == 1) { // Artist
                    cmp = mA.getArtist().compare(mB.getArtist());
                } else if (m_sortField == 2) { // Album
                    cmp = mA.getAlbum().compare(mB.getAlbum());
                } else if (m_sortField == 3) { // Duration
                    int dA = mA.getDuration();
                    int dB = mB.getDuration();
                    cmp = (dA < dB) ? -1 : ((dA > dB) ? 1 : 0);
                }
                return m_sortAscending ? (cmp < 0) : (cmp > 0);
            });
    }

    int filteredCount = static_cast<int>(filteredIndices.size());
    int totalPages = (filteredCount + ITEMS_PER_PAGE - 1) / ITEMS_PER_PAGE;
    if (totalPages == 0) totalPages = 1;

    // Header
    painter.drawText("Library", x, y, theme.textPrimary, 20);
    
    // Block input if context menu is open
    bool inputBlocked = m_showContextMenu;
    
    // Search Filters
    if (!m_searchQuery.empty()) {
        int filterX = x + 90;
        int filterY = y + 2;
        int filterBtnW = 55;
        int filterBtnH = 22;
        int filterGap = 5;
        
        const char* labels[] = {"All", "Title", "Artist", "Album"};
        
        for (int i = 0; i < 4; ++i) {
            bool selected = (m_searchFilter == i);
            bool hover = painter.isMouseOver(filterX, filterY, filterBtnW, filterBtnH);
            
            uint32_t btnColor = selected ? theme.primary : 
                               (hover ? theme.surfaceHover : theme.surface);
            painter.drawRect(filterX, filterY, filterBtnW, filterBtnH, btnColor);
            painter.drawRect(filterX, filterY, filterBtnW, filterBtnH, theme.border, false);
            
            uint32_t textColor = selected ? theme.textPrimary : theme.textSecondary;
            painter.drawText(labels[i], filterX + 8, filterY + 4, textColor, 11);
            
            if (!inputBlocked && hover && painter.isMouseClicked(filterX, filterY, filterBtnW, filterBtnH)) {
                m_searchFilter = i;
                m_currentPage = 0;
                painter.consumeClick();
            }
            filterX += filterBtnW + filterGap;
        }
        
        // Clear Search
        int clearBtnSize = 22;
        int clearX = filterX + 5;
        bool clearHover = painter.isMouseOver(clearX, filterY, clearBtnSize, clearBtnSize);
        if (clearHover) painter.drawRect(clearX, filterY, clearBtnSize, clearBtnSize, theme.surfaceActive);
        painter.drawRect(clearX, filterY, clearBtnSize, clearBtnSize, theme.border, false);
        painter.drawText("x", clearX + 7, filterY + 2, theme.textDim, 14);
        
        if (!inputBlocked && clearHover && painter.isMouseClicked(clearX, filterY, clearBtnSize, clearBtnSize)) {
            m_searchQuery.clear();
            painter.getState().searchQuery.clear(); // Sync to global state
            m_currentPage = 0;
            m_scrollOffset = 0;
            SDL_StopTextInput();
            painter.consumeClick();
        }
    }
    
    std::string pageInfo = "Page " + std::to_string(m_currentPage + 1) + "/" + std::to_string(totalPages);
    std::string countText = std::to_string(filteredCount) + " tracks";
    
    painter.drawText(countText, x + w - 180, y + 4, theme.textDim, 12);
    painter.drawText(pageInfo, x + w - 80, y + 4, theme.textSecondary, 12);
    
    y += 40;
    
    // Column Headers
    int colTitle = x;
    int colArtist = x + w * 0.35;
    int colAlbum = x + w * 0.6;
    int colDuration = x + w - 70;
    
    painter.drawRect(x, y, w, 30, theme.surface);
    
    auto drawSortHeader = [&](const std::string& label, int sortId, int colX, int colW) {
        bool hover = painter.isMouseOver(colX, y, colW, 30);
        std::string text = label;
        if (m_sortField == sortId) text += m_sortAscending ? " ^" : " v";
        painter.drawText(text, colX + (sortId == 0 ? 50 : 0), y + 7, hover ? theme.textPrimary : theme.textSecondary, 12);
        
        if (!inputBlocked && hover && painter.isMouseClicked(colX, y, colW, 30)) {
            if (m_sortField == sortId) m_sortAscending = !m_sortAscending;
            else { m_sortField = sortId; m_sortAscending = true; }
            painter.consumeClick();
        }
    };
    
    drawSortHeader("Title", 0, colTitle, static_cast<int>(w * 0.35));
    drawSortHeader("Artist", 1, colArtist, static_cast<int>(w * 0.25));
    drawSortHeader("Album", 2, colAlbum, static_cast<int>(w * 0.25));
    drawSortHeader("Time", 3, colDuration, 70);
    
    y += 30;
    
    // List Rendering
    int listH = h - 110;
    SDL_Rect clipRect = { x, y, w, listH };
    SDL_RenderSetClipRect(painter.getRenderer(), &clipRect);
    
    if (m_currentPage >= totalPages) m_currentPage = std::max(0, totalPages - 1);
    
    int startIndex = m_currentPage * ITEMS_PER_PAGE;
    int endIndex = std::min(startIndex + ITEMS_PER_PAGE, filteredCount);
    
    for (int i = startIndex; i < endIndex; i++) {
        size_t index = filteredIndices[i];
        const auto& media = m_currentMediaList[index];
        
        int rowIdx = i - startIndex;
        int itemY = y + (rowIdx * 50) - m_scrollOffset;
        
        if (itemY + 50 < y || itemY > y + listH) continue;
        
        // Hover/Selection Logic
        bool selected = (index == static_cast<size_t>(m_selectedIndex));
        bool hover = painter.isMouseOver(x, itemY, w, 50) && painter.isMouseOver(x, y, w, listH);
        
        uint32_t rowBg = selected ? theme.surfaceActive : 
                        (hover ? theme.surfaceHover : 
                        (i % 2 == 0 ? theme.background : theme.surface));
        painter.drawRect(x, itemY, w, 50, rowBg);
        
        // Icon
        const char* icon = media.isAudio() ? "~" : (media.isVideo() ? "*" : "?");
        uint32_t iconColor = media.isUnsupported() ? theme.textDim : theme.textSecondary;
        if (selected) { // Should check if playing? But Library doesn't know playing state easily without PlaybackStateModel
            // For now just show arrow if selected
            painter.drawText(">", x + 15, itemY + 15, theme.success, 14);
        } else {
            painter.drawText(icon, x + 15, itemY + 15, iconColor, 14);
        }

        // Text
        uint32_t textCol = media.isUnsupported() ? theme.textDim : theme.textPrimary;
        std::string title = media.getTitle().empty() ? media.getFileName() : media.getTitle();
        if (title.length() > 35) title = title.substr(0, 32) + "...";
        painter.drawText(title, colTitle + 50, itemY + 15, textCol, 14);
        
        std::string artist = media.getArtist().empty() ? "Unknown Artist" : media.getArtist();
        if (artist.length() > 25) artist = artist.substr(0, 22) + "...";
        painter.drawText(artist, colArtist, itemY + 15, theme.textSecondary, 14);
        
        std::string album = media.getAlbum();
        if (album.length() > 25) album = album.substr(0, 22) + "...";
        painter.drawText(album, colAlbum, itemY + 15, theme.textSecondary, 14);

        int duration = media.getDuration();
        std::string durationStr = "--:--";
        if (duration > 0) {
            int mins = duration / 60;
            int secs = duration % 60;
            std::ostringstream oss;
            oss << mins << ":" << std::setfill('0') << std::setw(2) << secs;
            durationStr = oss.str();
        }
        painter.drawText(durationStr, colDuration, itemY + 15, theme.textDim, 12);
        
        // Options Button (...)
        int optBtnSize = 30;
        int optBtnX = x + w - optBtnSize - 10; // Right aligned
        int optBtnY = itemY + 10;
        
        bool optHover = painter.isMouseOver(optBtnX, optBtnY, optBtnSize, optBtnSize);
        if (optHover) painter.drawRect(optBtnX, optBtnY, optBtnSize, optBtnSize, theme.surfaceActive);
        painter.drawText("...", optBtnX + 8, optBtnY + 2, theme.textSecondary, 16);

        // Click Handler
        if (!inputBlocked && hover) {
            int mx, my;
            SDL_GetMouseState(&mx, &my);
            
            // Options Button Click (Left Click) -> Context Menu
            bool optionsClicked = optHover && painter.isLeftMouseClicked(optBtnX, optBtnY, optBtnSize, optBtnSize);
            
            // Left Click on Row (Play Now behavior: Insert Next + Play Next)
            bool leftClickRow = painter.isLeftMouseClicked(x, itemY, w, 50);

            // Right Click on Row (Context Menu)
            bool rightClick = painter.isRightMouseClicked(x, itemY, w, 50);

            if (optionsClicked || rightClick) {
                m_selectedIndex = static_cast<int>(index);
                m_showContextMenu = true;
                m_contextMenuX = mx; 
                m_contextMenuY = my;
                m_contextMenuIndex = static_cast<int>(index);
                painter.consumeClick();
            }
            else if (leftClickRow) {
                 if (!media.isUnsupported()) {
                    m_selectedIndex = static_cast<int>(index);
                    if (m_playbackController && m_queueController) {
                        // "Play Now" behavior:
                        // 1. Check if song already in queue
                        // 2. If yes, jump to that position and play
                        // 3. If no, add as next and play
                        
                        // Check if already in queue
                        int existingIndex = -1;
                        auto queueItems = m_queueController->getAllItems();
                        for (size_t qi = 0; qi < queueItems.size(); ++qi) {
                            if (queueItems[qi].getFilePath() == media.getFilePath()) {
                                existingIndex = static_cast<int>(qi);
                                break;
                            }
                        }
                        
                        if (existingIndex >= 0) {
                            // Song already in queue - jump to it and play
                            m_queueController->jumpToIndex(static_cast<size_t>(existingIndex));
                            m_playbackController->play();
                        } else if (m_queueController->isEmpty()) {
                            m_queueController->addToQueue(media);
                            m_playbackController->play();
                        } else {
                            // Add as next item and play
                            size_t nextIdx = m_queueController->getCurrentIndex() + 1;
                            m_queueController->addToQueueNext(media);
                            m_queueController->jumpToIndex(nextIdx);
                            m_playbackController->play();
                        }
                    }
                 }
                 painter.consumeClick();
            }
        }
    }
    
    SDL_RenderSetClipRect(painter.getRenderer(), nullptr);
    
    // Pagination Buttons
    int pageBtnsY = y + listH + 10;
    int prevBtnX = x + w/2 - 100;
    int nextBtnX = x + w/2 + 20;

    if (m_currentPage > 0) {
        bool prevHover = painter.isMouseOver(prevBtnX, pageBtnsY, 80, 28);
        painter.drawRect(prevBtnX, pageBtnsY, 80, 28, prevHover ? theme.primaryHover : theme.primary);
        painter.drawText("< Prev", prevBtnX + 15, pageBtnsY + 6, theme.textPrimary, 12);
        if (!inputBlocked && prevHover && painter.isMouseClicked(prevBtnX, pageBtnsY, 80, 28)) {
             m_currentPage--;
             painter.consumeClick();
        }
    }

    if (m_currentPage < totalPages - 1) {
        bool nextHover = painter.isMouseOver(nextBtnX, pageBtnsY, 80, 28);
        painter.drawRect(nextBtnX, pageBtnsY, 80, 28, nextHover ? theme.primaryHover : theme.primary);
        painter.drawText("Next >", nextBtnX + 15, pageBtnsY + 6, theme.textPrimary, 12);
        if (!inputBlocked && nextHover && painter.isMouseClicked(nextBtnX, pageBtnsY, 80, 28)) {
             m_currentPage++;
             painter.consumeClick();
        }
    }
    
    // Context Menu Rendering
    if (m_showContextMenu) {
         int mx = m_contextMenuX;
         int my = m_contextMenuY;
         int mw = 160;
         int mh = 150; // 4 items * 35 + 10
         
         // Ensure menu stays on screen
         if (mx + mw > painter.getWidth()) mx -= mw;
         if (my + mh > painter.getHeight()) my -= mh;
         
         // Close if clicked outside (use correct menu dimensions)
         if (painter.isMouseClicked(0, 0, painter.getWidth(), painter.getHeight()) && 
             !painter.isMouseOver(mx, my, mw, mh)) {
             m_showContextMenu = false;
             painter.consumeClick();
             return;
         }
         
         // Draw Menu Background
         painter.drawRect(mx, my, mw, mh, theme.surface);
         painter.drawRect(mx, my, mw, mh, theme.border, false); // Border
         
         // Menu Items
         const char* menuItems[] = {"Add to Queue", "Play Next", "Add to Playlist", "Properties"};
         int itemH = 35;
         int iy = my + 5;
         
         for (int i = 0; i < 4; i++) {
             bool itemHover = painter.isMouseOver(mx, iy, mw, itemH);
             if (itemHover) {
                 painter.drawRect(mx, iy, mw, itemH, theme.surfaceHover);
                 if (painter.isMouseClicked(mx, iy, mw, itemH)) {
                     if (m_contextMenuIndex >= 0 && m_contextMenuIndex < static_cast<int>(m_currentMediaList.size())) {
                         // Use absolute index directly
                         const auto& targetMedia = m_currentMediaList[m_contextMenuIndex];
                         
                         if (i == 0) { // Add to Queue
                             if (m_queueController) m_queueController->addToQueue(targetMedia);
                         } else if (i == 1) { // Play Next
                             if (m_queueController) {
                                 m_queueController->addToQueueNext(targetMedia); 
                             }
                         } else if (i == 2) { // Add to Playlist
                             if (m_playlistController) {
                                 // Open Add to Playlist Dialog
                                 auto& state = painter.getState();
                                 state.showAddToPlaylistDialog = true;
                                 state.contextMediaItem = targetMedia;
                             }
                          } else if (i == 3) { // Properties
                             // Hiển thị Properties dialog
                             auto& state = painter.getState();
                             state.contextMediaItem = targetMedia;
                             state.showPropertiesDialog = true;
                             
                             // Điền metadata vào state
                             state.metadataEdit.filePath = targetMedia.getFilePath();
                             state.metadataEdit.fileName = targetMedia.getFileName();
                             state.metadataEdit.extension = targetMedia.getExtension();
                             state.metadataEdit.typeStr = targetMedia.isAudio() ? "Audio" : 
                                 (targetMedia.isVideo() ? "Video" : 
                                 (targetMedia.isUnsupported() ? "Unsupported" : "Unknown"));
                             
                             // File size
                             size_t sz = targetMedia.getFileSize();
                             if (sz >= 1024 * 1024)
                                 state.metadataEdit.fileSizeStr = std::to_string(sz / (1024 * 1024)) + " MB";
                             else if (sz >= 1024)
                                 state.metadataEdit.fileSizeStr = std::to_string(sz / 1024) + " KB";
                             else
                                 state.metadataEdit.fileSizeStr = std::to_string(sz) + " B";
                             
                             // Duration (fallback from media file)
                             int dur = targetMedia.getDuration();
                             state.metadataEdit.durationStr = (dur > 0) ? 
                                 (std::to_string(dur / 60) + ":" + (dur % 60 < 10 ? "0" : "") + std::to_string(dur % 60)) : "-";
                             
                             // Basic metadata từ MediaFileModel
                             state.metadataEdit.title = targetMedia.getTitle().empty() ? 
                                 targetMedia.getFileName() : targetMedia.getTitle();
                             state.metadataEdit.artist = targetMedia.getArtist().empty() ? "-" : targetMedia.getArtist();
                             state.metadataEdit.album = targetMedia.getAlbum().empty() ? "-" : targetMedia.getAlbum();
                             state.metadataEdit.genre = "-";
                             state.metadataEdit.year = "-";
                             state.metadataEdit.publisher = "-";
                             state.metadataEdit.bitrateStr = "-";
                             
                             // Đọc metadata chi tiết từ LibraryController
                             if (m_libraryController && !targetMedia.isUnsupported()) {
                                 auto meta = m_libraryController->readMetadata(targetMedia.getFilePath());
                                 if (meta) {
                                     if (!meta->getTitle().empty()) state.metadataEdit.title = meta->getTitle();
                                     if (!meta->getArtist().empty()) state.metadataEdit.artist = meta->getArtist();
                                     if (!meta->getAlbum().empty()) state.metadataEdit.album = meta->getAlbum();
                                     if (!meta->getGenre().empty()) state.metadataEdit.genre = meta->getGenre();
                                     if (!meta->getYear().empty()) state.metadataEdit.year = meta->getYear();
                                     if (!meta->getPublisher().empty()) state.metadataEdit.publisher = meta->getPublisher();
                                     state.metadataEdit.durationStr = meta->getFormattedDuration();
                                     if (meta->getDuration() <= 0 && dur > 0)
                                         state.metadataEdit.durationStr = std::to_string(dur / 60) + ":" + (dur % 60 < 10 ? "0" : "") + std::to_string(dur % 60);
                                     if (meta->getBitrate() > 0) 
                                         state.metadataEdit.bitrateStr = std::to_string(meta->getBitrate()) + " kbps";
                                 }
                             }
                          }
                     }
                     m_showContextMenu = false;
                     painter.consumeClick();
                 }
             }
             painter.drawText(menuItems[i], mx + 15, iy + 8, theme.textPrimary, 14);
             iy += itemH;
         }
    }
}

bool LibraryScreen::handleInput(const SDL_Event& event) 
{
    // Block input if context menu is open
    if (m_showContextMenu) {
        return true; // Consume event so it doesn't propagate
    }

    // Handle mouse wheel for scrolling within page
    if (event.type == SDL_MOUSEWHEEL) {
        m_scrollOffset -= event.wheel.y * 30; // 30 pixels per scroll step
        if (m_scrollOffset < 0) m_scrollOffset = 0;
        
        // Calculate max scroll based on items in current page
        int itemsInPage = std::min(ITEMS_PER_PAGE, static_cast<int>(m_currentMediaList.size()) - m_currentPage * ITEMS_PER_PAGE);
        int contentHeight = itemsInPage * 50; // 50px per item
        int listHeight = 400; // Approximate visible list height (adjust based on actual layout)
        int maxScroll = std::max(0, contentHeight - listHeight);
        if (m_scrollOffset > maxScroll) m_scrollOffset = maxScroll;
        
        return true;
    }

    if (event.type == SDL_TEXTINPUT) {
        // Handle search query input
        // Only if we decide search box focus is handled here.
        // ImGuiManager handled search focus globally.
        // We need to know if search is focused.
        // Let's implement search focus logic inside render (checking click on search box).
        // For now, accept text input if reasonable?
        // Actually, ImGuiManager handled `m_state.searchFocused`.
        // We need to track `m_searchFocused` locally.
        
        m_searchQuery += event.text.text;
        m_currentPage = 0;
        m_scrollOffset = 0; // Reset scroll when searching
        return true;
    }
    else if (event.type == SDL_KEYDOWN) {
        if (event.key.keysym.sym == SDLK_BACKSPACE && !m_searchQuery.empty()) {
            m_searchQuery.pop_back();
            m_currentPage = 0;
            m_scrollOffset = 0; // Reset scroll when searching
            return true;
        }
        
        // Navigation keys (Up/Down) handled here...
        if (event.key.keysym.sym == SDLK_UP) {
            if (m_selectedIndex > 0) m_selectedIndex--;
            // Scroll logic...
            return true;
        }
        else if (event.key.keysym.sym == SDLK_DOWN) {
            if (m_selectedIndex < static_cast<int>(m_currentMediaList.size()) - 1) m_selectedIndex++;
            return true;
        }
    }
    
    return false;
}

// ... Keep other methods ...

void LibraryScreen::nextPage() 
{
    // ...
}

void LibraryScreen::previousPage() 
{
    // ...
}

void LibraryScreen::goToPage(size_t /*pageNumber*/) 
{
    // ...
}

void LibraryScreen::showAllMedia() 
{
    // ...
}

void LibraryScreen::showAudioOnly() 
{
    // ...
}

void LibraryScreen::showVideoOnly() 
{
    // ...
}

void LibraryScreen::searchMedia(const std::string& /*query*/) 
{
    // ...
}

void LibraryScreen::sortByTitle() 
{
    // ...
}

void LibraryScreen::sortByArtist() 
{
    // ...
}

void LibraryScreen::sortByAlbum() 
{
    // ...
}

void LibraryScreen::playSelected() 
{
    // ...
}

void LibraryScreen::addSelectedToQueue() 
{
    // ...
}

void LibraryScreen::addAllToQueue() 
{
    // ...
}

void LibraryScreen::refreshMediaList() 
{
    m_currentMediaList = m_libraryController->getAllMedia();
    m_currentPage = 0;
    // Reset sort/filter?
}

 

} // namespace views
} // namespace media_player
