// Project includes
#include "views/ExploreScreen.h"
#include "controllers/ExploreController.h"
#include "controllers/LibraryController.h"
#include "models/ExploreModel.h"
#include "ui/ImGuiManager.h"

// System includes
#include <algorithm>
#include <iomanip>
#include <sstream>
#include <filesystem>

namespace fs = std::filesystem;

namespace media_player 
{
namespace views 
{

// ============================================================================
// Constructor / Destructor
// ============================================================================

ExploreScreen::ExploreScreen(
    std::shared_ptr<controllers::ExploreController> exploreController,
    std::shared_ptr<models::ExploreModel> exploreModel
)
    : m_exploreController(exploreController)
    , m_exploreModel(exploreModel)
    , m_isVisible(false)
    , m_scrollOffset(0)
    , m_showContextMenu(false)
    , m_contextMenuX(0)
    , m_contextMenuY(0)
    , m_contextMenuIndex(-1)
{
}

ExploreScreen::~ExploreScreen() 
{
}

// ============================================================================
// IView Interface
// ============================================================================

void ExploreScreen::show() 
{
    m_isVisible = true;
    
    // Yêu cầu controller tải lại media list
    if (m_exploreController) 
    {
        m_exploreController->refreshMediaList();
    }
}

void ExploreScreen::hide() 
{
    m_isVisible = false;
}

void ExploreScreen::update() 
{
    // Không cần update mỗi frame
}

bool ExploreScreen::isVisible() const 
{
    return m_isVisible;
}

// ============================================================================
// Render — chỉ vẽ UI, delegate logic xuống controller
// ============================================================================

void ExploreScreen::render(ui::ImGuiManager& painter) 
{
    // Layout dimensions
    int x = ui::ImGuiManager::GetSidebarWidth() + 10;
    int y = ui::ImGuiManager::GetMenuBarHeight() + 10;
    int w = painter.getWidth() - ui::ImGuiManager::GetSidebarWidth() - 20;
    int h = painter.getHeight() - ui::ImGuiManager::GetMenuBarHeight() 
            - ui::ImGuiManager::GetPlayerBarHeight() - 20;
    
    const auto& theme = painter.getTheme();
    
    // Đồng bộ search query từ global state
    if (m_searchQuery != painter.getState().searchQuery) 
    {
        m_searchQuery = painter.getState().searchQuery;
    }
    
    bool inputBlocked = m_showContextMenu;
    
    // Header: "Explore"
    painter.drawText("Explore", x, y, theme.textPrimary, 20);
    
    // Hiển thị số items (đọc từ controller)
    std::string countText = std::to_string(m_exploreController->getFolderCount()) + " folders, " 
                            + std::to_string(m_exploreController->getFileCount()) + " tracks";
    painter.drawText(countText, x + w - 200, y + 4, theme.textDim, 12);
    
    y += 30;
    
    // Breadcrumb navigation
    renderBreadcrumb(painter, x, y, w);
    y += 35;
    
    // Nội dung danh sách (folder + file)
    int listH = h - 85; // Trừ header + breadcrumb
    SDL_Rect clipRect = { x, y, w, listH };
    SDL_RenderSetClipRect(painter.getRenderer(), &clipRect);
    
    int renderY = y - m_scrollOffset;
    
    // Lấy danh sách đã lọc từ controller
    auto filteredFolders = m_exploreController->getFilteredFolders(m_searchQuery);
    auto filteredFileIndices = m_exploreController->getFilteredFileIndices(m_searchQuery);
    
    // ====== Render folder list ======
    for (size_t fi = 0; fi < filteredFolders.size(); fi++) 
    {
        const auto& folder = filteredFolders[fi];
        int itemH = 45;
        int itemY = renderY;
        
        // Bỏ qua nếu ngoài viewport
        if (itemY + itemH < y || itemY > y + listH) 
        {
            renderY += itemH;
            continue;
        }
        
        bool hover = painter.isMouseOver(x, itemY, w, itemH) 
                     && painter.isMouseOver(x, y, w, listH);
        
        // Background
        uint32_t rowBg = hover ? theme.surfaceHover : 
                         (fi % 2 == 0 ? theme.background : theme.surface);
        painter.drawRect(x, itemY, w, itemH, rowBg);
        
        // Folder icon
        painter.drawText("[D]", x + 12, itemY + 12, theme.warning, 14);
        
        // Folder name
        std::string displayName = folder.name;
        if (displayName.length() > 40) 
        {
            displayName = displayName.substr(0, 37) + "...";
        }
        painter.drawText(displayName, x + 50, itemY + 8, theme.textPrimary, 14);
        
        // File count
        std::string fileCountStr = std::to_string(folder.fileCount) + " tracks";
        painter.drawText(fileCountStr, x + 50, itemY + 26, theme.textDim, 11);
        
        // Arrow indicator (>)
        painter.drawText(">", x + w - 30, itemY + 12, theme.textSecondary, 14);
        
        // Click handler — delegate navigate xuống controller
        if (!inputBlocked && hover 
            && painter.isLeftMouseClicked(x, itemY, w, itemH)) 
        {
            m_exploreController->navigateToFolder(folder.fullPath);
            m_scrollOffset = 0;
            painter.consumeClick();
            SDL_RenderSetClipRect(painter.getRenderer(), nullptr);
            return; // View đã thay đổi, thoát render frame này
        }
        
        renderY += itemH;
    }
    
    // Separator giữa folder và file (nếu có cả hai)
    if (!filteredFolders.empty() && !filteredFileIndices.empty()) 
    {
        painter.drawRect(x + 10, renderY + 2, w - 20, 1, theme.border);
        renderY += 8;
    }
    
    // ====== Render file list ======
    const auto& currentFiles = m_exploreModel->getCurrentFiles();
    
    for (size_t fi = 0; fi < filteredFileIndices.size(); fi++) 
    {
        size_t fileIdx = filteredFileIndices[fi];
        
        // Kiểm tra index hợp lệ
        if (fileIdx >= currentFiles.size()) 
        {
            continue;
        }
        
        const auto& media = currentFiles[fileIdx];
        int itemH = 50;
        int itemY = renderY;
        
        // Bỏ qua nếu ngoài viewport
        if (itemY + itemH < y || itemY > y + listH) 
        {
            renderY += itemH;
            continue;
        }
        
        bool hover = painter.isMouseOver(x, itemY, w, itemH) 
                     && painter.isMouseOver(x, y, w, listH);
        
        // Background
        uint32_t rowBg = hover ? theme.surfaceHover : 
                         (fi % 2 == 0 ? theme.background : theme.surface);
        painter.drawRect(x, itemY, w, itemH, rowBg);
        
        // Music icon
        const char* icon = media.isAudio() ? "~" : (media.isVideo() ? "*" : "?");
        painter.drawText(icon, x + 15, itemY + 15, theme.textSecondary, 14);
        
        // Title
        int colTitle = x + 50;
        int colArtist = x + static_cast<int>(w * 0.45);
        int colDuration = x + w - 70;
        
        std::string title = media.getTitle().empty() ? media.getFileName() : media.getTitle();
        if (title.length() > 40) 
        {
            title = title.substr(0, 37) + "...";
        }
        uint32_t textCol = media.isUnsupported() ? theme.textDim : theme.textPrimary;
        painter.drawText(title, colTitle, itemY + 15, textCol, 14);
        
        // Artist
        std::string artist = media.getArtist().empty() ? "Unknown Artist" : media.getArtist();
        if (artist.length() > 25) 
        {
            artist = artist.substr(0, 22) + "...";
        }
        painter.drawText(artist, colArtist, itemY + 15, theme.textSecondary, 14);
        
        // Duration
        int duration = media.getDuration();
        std::string durationStr = "--:--";
        if (duration > 0) 
        {
            int mins = duration / 60;
            int secs = duration % 60;
            std::ostringstream oss;
            oss << mins << ":" << std::setfill('0') << std::setw(2) << secs;
            durationStr = oss.str();
        }
        painter.drawText(durationStr, colDuration, itemY + 15, theme.textDim, 12);
        
        // Options button (...)
        int optBtnSize = 30;
        int optBtnX = x + w - optBtnSize - 10;
        int optBtnY = itemY + 10;
        
        bool optHover = painter.isMouseOver(optBtnX, optBtnY, optBtnSize, optBtnSize);
        if (optHover) 
        {
            painter.drawRect(optBtnX, optBtnY, optBtnSize, optBtnSize, theme.surfaceActive);
        }
        painter.drawText("...", optBtnX + 8, optBtnY + 2, theme.textSecondary, 16);
        
        // Click handler — delegate actions xuống controller
        if (!inputBlocked && hover) 
        {
            int mx, my;
            SDL_GetMouseState(&mx, &my);
            
            bool optionsClicked = optHover 
                && painter.isLeftMouseClicked(optBtnX, optBtnY, optBtnSize, optBtnSize);
            bool leftClickRow = painter.isLeftMouseClicked(x, itemY, w, itemH);
            bool rightClick = painter.isRightMouseClicked(x, itemY, w, itemH);
            
            if (optionsClicked || rightClick) 
            {
                // Mở context menu (UI state)
                m_showContextMenu = true;
                m_contextMenuX = mx;
                m_contextMenuY = my;
                m_contextMenuIndex = static_cast<int>(fileIdx);
                painter.consumeClick();
            }
            else if (leftClickRow) 
            {
                // Play Now — delegate xuống controller
                m_exploreController->playFile(fileIdx);
                painter.consumeClick();
            }
        }
        
        renderY += itemH;
    }
    
    SDL_RenderSetClipRect(painter.getRenderer(), nullptr);
    
    // Render context menu (trên cùng, không bị clip)
    if (m_showContextMenu) 
    {
        renderContextMenu(painter);
    }
}

// ============================================================================
// Breadcrumb — render + delegate navigation xuống controller
// ============================================================================

void ExploreScreen::renderBreadcrumb(ui::ImGuiManager& painter, int x, int y, int w) 
{
    const auto& theme = painter.getTheme();
    
    // Đọc path từ controller
    std::string currentPath = m_exploreController->getCurrentPath();
    std::string rootPath = m_exploreController->getRootPath();
    bool isAtRoot = m_exploreController->isAtRoot();
    
    // Background bar
    painter.drawRect(x, y, w, 28, theme.surface);
    
    int textX = x + 10;
    
    // Back button (nếu không ở root)
    if (!isAtRoot && !currentPath.empty()) 
    {
        int backBtnW = 50;
        int backBtnH = 24;
        int backBtnY = y + 2;
        
        bool backHover = painter.isMouseOver(textX, backBtnY, backBtnW, backBtnH);
        painter.drawRect(textX, backBtnY, backBtnW, backBtnH, 
                         backHover ? theme.primaryHover : theme.primary);
        painter.drawText("< Back", textX + 5, backBtnY + 4, theme.textPrimary, 12);
        
        if (backHover && painter.isMouseClicked(textX, backBtnY, backBtnW, backBtnH)) 
        {
            m_exploreController->navigateUp();
            m_scrollOffset = 0;
            painter.consumeClick();
            return;
        }
        
        textX += backBtnW + 10;
    }
    
    // Tạo breadcrumb segments từ path
    if (currentPath.length() >= rootPath.length()) 
    {
        // Root segment
        std::string rootName = fs::path(rootPath).filename().string();
        if (rootName.empty()) 
        {
            rootName = rootPath;
        }
        
        uint32_t rootColor = isAtRoot ? theme.textPrimary : theme.primary;
        
        // Click root → delegate navigate về root
        int rootW = static_cast<int>(rootName.length()) * 8 + 10;
        bool rootHover = !isAtRoot && painter.isMouseOver(textX, y, rootW, 28);
        
        painter.drawText(rootName, textX, y + 6, 
                         rootHover ? theme.primaryHover : rootColor, 13);
        
        if (rootHover && painter.isMouseClicked(textX, y, rootW, 28)) 
        {
            m_exploreController->navigateToRoot();
            m_scrollOffset = 0;
            painter.consumeClick();
            return;
        }
        
        textX += rootW;
        
        // Sub-segments (nếu đang trong subfolder)
        if (!isAtRoot) 
        {
            std::string relative = currentPath.substr(rootPath.length());
            if (!relative.empty() && relative[0] == '/') 
            {
                relative = relative.substr(1);
            }
            
            std::istringstream iss(relative);
            std::string segment;
            std::string builtPath = rootPath;
            
            while (std::getline(iss, segment, '/')) 
            {
                if (segment.empty()) 
                {
                    continue;
                }
                
                builtPath += "/" + segment;
                
                // Separator " > "
                painter.drawText(" > ", textX, y + 6, theme.textDim, 13);
                textX += 28;
                
                bool isLast = (builtPath == currentPath);
                int segW = static_cast<int>(segment.length()) * 8 + 10;
                
                // Giới hạn breadcrumb không tràn quá width
                if (textX + segW > x + w - 20) 
                {
                    painter.drawText("...", textX, y + 6, theme.textDim, 13);
                    break;
                }
                
                // Click segment → delegate navigate tới folder đó
                bool segHover = !isLast && painter.isMouseOver(textX, y, segW, 28);
                uint32_t segColor = isLast ? theme.textPrimary : 
                                    (segHover ? theme.primaryHover : theme.primary);
                
                painter.drawText(segment, textX, y + 6, segColor, 13);
                
                if (segHover && painter.isMouseClicked(textX, y, segW, 28)) 
                {
                    m_exploreController->navigateToBreadcrumb(builtPath);
                    m_scrollOffset = 0;
                    painter.consumeClick();
                    return;
                }
                
                textX += segW;
            }
        }
    }
}

// ============================================================================
// Context Menu — render + delegate actions xuống controller
// ============================================================================

void ExploreScreen::renderContextMenu(ui::ImGuiManager& painter) 
{
    const auto& theme = painter.getTheme();
    
    int mx = m_contextMenuX;
    int my = m_contextMenuY;
    int mw = 160;
    int mh = 150; // 4 items * 35 + 10
    
    // Đảm bảo menu không tràn ra ngoài màn hình
    if (mx + mw > painter.getWidth()) 
    {
        mx -= mw;
    }
    if (my + mh > painter.getHeight()) 
    {
        my -= mh;
    }
    
    // Đóng nếu click bên ngoài menu
    if (painter.isMouseClicked(0, 0, painter.getWidth(), painter.getHeight()) 
        && !painter.isMouseOver(mx, my, mw, mh)) 
    {
        m_showContextMenu = false;
        painter.consumeClick();
        return;
    }
    
    // Menu background
    painter.drawRect(mx, my, mw, mh, theme.surface);
    painter.drawRect(mx, my, mw, mh, theme.border, false);
    
    // Menu items
    const char* menuItems[] = {"Add to Queue", "Play Next", "Add to Playlist", "Properties"};
    int itemH = 35;
    int iy = my + 5;
    
    for (int i = 0; i < 4; i++) 
    {
        bool itemHover = painter.isMouseOver(mx, iy, mw, itemH);
        
        if (itemHover) 
        {
            painter.drawRect(mx, iy, mw, itemH, theme.surfaceHover);
            
            if (painter.isMouseClicked(mx, iy, mw, itemH)) 
            {
                const auto* targetMedia = m_exploreController->getFileAt(
                    static_cast<size_t>(m_contextMenuIndex));
                
                if (targetMedia) 
                {
                    if (i == 0) 
                    {
                        // Add to Queue — delegate controller
                        m_exploreController->addToQueue(
                            static_cast<size_t>(m_contextMenuIndex));
                    }
                    else if (i == 1) 
                    {
                        // Play Next — delegate controller
                        m_exploreController->addToQueueNext(
                            static_cast<size_t>(m_contextMenuIndex));
                    }
                    else if (i == 2) 
                    {
                        // Add to Playlist — set UI state
                        auto& state = painter.getState();
                        state.showAddToPlaylistDialog = true;
                        state.contextMediaItem = *targetMedia;
                    }
                    else if (i == 3) 
                    {
                        // Properties — set UI state + đọc metadata qua controller
                        auto& state = painter.getState();
                        state.contextMediaItem = *targetMedia;
                        state.showPropertiesDialog = true;
                        
                        // Điền metadata cơ bản
                        state.metadataEdit.filePath = targetMedia->getFilePath();
                        state.metadataEdit.fileName = targetMedia->getFileName();
                        state.metadataEdit.extension = targetMedia->getExtension();
                        state.metadataEdit.typeStr = targetMedia->isAudio() ? "Audio" : 
                            (targetMedia->isVideo() ? "Video" : 
                            (targetMedia->isUnsupported() ? "Unsupported" : "Unknown"));
                        
                        // File size
                        size_t sz = targetMedia->getFileSize();
                        if (sz >= 1024 * 1024)
                        {
                            state.metadataEdit.fileSizeStr = std::to_string(sz / (1024 * 1024)) + " MB";
                        }
                        else if (sz >= 1024)
                        {
                            state.metadataEdit.fileSizeStr = std::to_string(sz / 1024) + " KB";
                        }
                        else
                        {
                            state.metadataEdit.fileSizeStr = std::to_string(sz) + " B";
                        }
                        
                        // Duration
                        int dur = targetMedia->getDuration();
                        state.metadataEdit.durationStr = (dur > 0) ? 
                            (std::to_string(dur / 60) + ":" + (dur % 60 < 10 ? "0" : "") 
                             + std::to_string(dur % 60)) : "-";
                        
                        // Metadata từ MediaFileModel
                        state.metadataEdit.title = targetMedia->getTitle().empty() ? 
                            targetMedia->getFileName() : targetMedia->getTitle();
                        state.metadataEdit.artist = targetMedia->getArtist().empty() ? 
                            "-" : targetMedia->getArtist();
                        state.metadataEdit.album = targetMedia->getAlbum().empty() ? 
                            "-" : targetMedia->getAlbum();
                        state.metadataEdit.genre = "-";
                        state.metadataEdit.year = "-";
                        state.metadataEdit.publisher = "-";
                        state.metadataEdit.bitrateStr = "-";
                        
                        // Đọc metadata chi tiết qua controller → LibraryController
                        auto libCtrl = m_exploreController->getLibraryController();
                        if (libCtrl && !targetMedia->isUnsupported()) 
                        {
                            auto meta = libCtrl->readMetadata(targetMedia->getFilePath());
                            if (meta) 
                            {
                                if (!meta->getTitle().empty()) 
                                {
                                    state.metadataEdit.title = meta->getTitle();
                                }
                                if (!meta->getArtist().empty()) 
                                {
                                    state.metadataEdit.artist = meta->getArtist();
                                }
                                if (!meta->getAlbum().empty()) 
                                {
                                    state.metadataEdit.album = meta->getAlbum();
                                }
                                if (!meta->getGenre().empty()) 
                                {
                                    state.metadataEdit.genre = meta->getGenre();
                                }
                                if (!meta->getYear().empty()) 
                                {
                                    state.metadataEdit.year = meta->getYear();
                                }
                                if (!meta->getPublisher().empty()) 
                                {
                                    state.metadataEdit.publisher = meta->getPublisher();
                                }
                                state.metadataEdit.durationStr = meta->getFormattedDuration();
                                if (meta->getDuration() <= 0 && dur > 0)
                                {
                                    state.metadataEdit.durationStr = std::to_string(dur / 60) + ":" 
                                        + (dur % 60 < 10 ? "0" : "") + std::to_string(dur % 60);
                                }
                                if (meta->getBitrate() > 0)
                                {
                                    state.metadataEdit.bitrateStr = std::to_string(meta->getBitrate()) + " kbps";
                                }
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

// ============================================================================
// Input Handling
// ============================================================================

bool ExploreScreen::handleInput(const SDL_Event& event) 
{
    // Block input nếu context menu đang mở
    if (m_showContextMenu) 
    {
        return true;
    }
    
    // Mouse wheel scroll
    if (event.type == SDL_MOUSEWHEEL) 
    {
        m_scrollOffset -= event.wheel.y * 30;
        if (m_scrollOffset < 0) 
        {
            m_scrollOffset = 0;
        }
        
        // Tính max scroll từ controller data
        int totalHeight = static_cast<int>(m_exploreController->getFolderCount()) * 45 
                       + static_cast<int>(m_exploreController->getFileCount()) * 50;
        int listH = 500; // Ước lượng chiều cao visible
        int maxScroll = std::max(0, totalHeight - listH);
        
        if (m_scrollOffset > maxScroll) 
        {
            m_scrollOffset = maxScroll;
        }
        
        return true;
    }
    
    return false;
}

} // namespace views
} // namespace media_player
