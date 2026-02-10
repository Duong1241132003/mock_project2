// Project includes
#include "views/ExploreScreen.h"
#include "config/AppConfig.h"
#include "ui/ImGuiManager.h"

// System includes
#include <algorithm>
#include <iomanip>
#include <sstream>
#include <filesystem>
#include <set>

namespace fs = std::filesystem;

namespace media_player 
{
namespace views 
{

// ============================================================================
// Constructor / Destructor
// ============================================================================

ExploreScreen::ExploreScreen(
    std::shared_ptr<controllers::LibraryController> libraryController,
    std::shared_ptr<controllers::QueueController> queueController,
    std::shared_ptr<controllers::PlaybackController> playbackController,
    std::shared_ptr<controllers::PlaylistController> playlistController
)
    : m_libraryController(libraryController)
    , m_queueController(queueController)
    , m_playbackController(playbackController)
    , m_playlistController(playlistController)
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
    
    // Tải lại toàn bộ media list từ controller
    if (m_libraryController) 
    {
        m_allMedia = m_libraryController->getAllMedia();
    }
    
    // Nếu chưa có current path, dùng root
    if (m_currentPath.empty() && !m_rootPath.empty()) 
    {
        m_currentPath = m_rootPath;
    }
    
    buildCurrentView();
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
// Root Path
// ============================================================================

void ExploreScreen::setRootPath(const std::string& rootPath) 
{
    m_rootPath = rootPath;
    m_currentPath = rootPath;
    m_pathStack.clear();
    m_scrollOffset = 0;
    
    // Tải lại media list
    if (m_libraryController) 
    {
        m_allMedia = m_libraryController->getAllMedia();
    }
    
    buildCurrentView();
}

// ============================================================================
// Navigation
// ============================================================================

void ExploreScreen::navigateToFolder(const std::string& folderPath) 
{
    // Lưu path hiện tại vào stack để back
    m_pathStack.push_back(m_currentPath);
    m_currentPath = folderPath;
    m_scrollOffset = 0;
    m_showContextMenu = false;
    
    buildCurrentView();
}

void ExploreScreen::navigateUp() 
{
    if (!m_pathStack.empty()) 
    {
        m_currentPath = m_pathStack.back();
        m_pathStack.pop_back();
    }
    else if (m_currentPath != m_rootPath) 
    {
        // Fallback: navigate lên parent directory
        fs::path parentPath = fs::path(m_currentPath).parent_path();
        
        // Không navigate ra ngoài root
        if (parentPath.string().length() >= m_rootPath.length()) 
        {
            m_currentPath = parentPath.string();
        }
    }
    
    m_scrollOffset = 0;
    m_showContextMenu = false;
    
    buildCurrentView();
}

// ============================================================================
// Xây dựng danh sách subfolder + file cho folder hiện tại
// ============================================================================

void ExploreScreen::buildCurrentView() 
{
    m_currentFolders.clear();
    m_currentFiles.clear();
    
    if (m_currentPath.empty()) 
    {
        return;
    }
    
    // Đảm bảo currentPath kết thúc bằng '/'
    std::string prefix = m_currentPath;
    if (!prefix.empty() && prefix.back() != '/') 
    {
        prefix += '/';
    }
    
    // Dùng set để thu thập tên subfolder (không trùng lặp)
    std::set<std::string> subfolderNames;
    
    for (const auto& media : m_allMedia) 
    {
        const std::string& filePath = media.getFilePath();
        
        // Kiểm tra file có thuộc folder hiện tại (hoặc subfolder) không
        if (filePath.length() <= prefix.length()) 
        {
            continue;
        }
        
        if (filePath.compare(0, prefix.length(), prefix) != 0) 
        {
            continue;
        }
        
        // Phần còn lại sau prefix
        std::string remaining = filePath.substr(prefix.length());
        
        // Tìm '/' tiếp theo
        size_t slashPos = remaining.find('/');
        
        if (slashPos == std::string::npos) 
        {
            // File nằm trực tiếp trong folder hiện tại
            m_currentFiles.push_back(media);
        }
        else 
        {
            // File nằm trong subfolder
            std::string subfolderName = remaining.substr(0, slashPos);
            subfolderNames.insert(subfolderName);
        }
    }
    
    // Tạo FolderEntry cho mỗi subfolder
    for (const auto& name : subfolderNames) 
    {
        FolderEntry entry;
        entry.name = name;
        entry.fullPath = prefix + name;
        
        // Đếm số file trong subfolder (đệ quy)
        std::string subPrefix = entry.fullPath + "/";
        entry.fileCount = 0;
        
        for (const auto& media : m_allMedia) 
        {
            if (media.getFilePath().compare(0, subPrefix.length(), subPrefix) == 0) 
            {
                entry.fileCount++;
            }
        }
        
        m_currentFolders.push_back(entry);
    }
    
    // Sắp xếp folder theo tên (alphabetical)
    std::sort(m_currentFolders.begin(), m_currentFolders.end(),
        [](const FolderEntry& a, const FolderEntry& b) 
        {
            return a.name < b.name;
        });
    
    // Sắp xếp file theo tên
    std::sort(m_currentFiles.begin(), m_currentFiles.end(),
        [](const models::MediaFileModel& a, const models::MediaFileModel& b) 
        {
            std::string nameA = a.getTitle().empty() ? a.getFileName() : a.getTitle();
            std::string nameB = b.getTitle().empty() ? b.getFileName() : b.getTitle();
            return nameA < nameB;
        });
}

// ============================================================================
// Render
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
    
    // Hiển thị số items
    int totalItems = static_cast<int>(m_currentFolders.size() + m_currentFiles.size());
    std::string countText = std::to_string(m_currentFolders.size()) + " folders, " 
                            + std::to_string(m_currentFiles.size()) + " tracks";
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
    
    // Lọc theo search query (nếu có)
    std::vector<FolderEntry> filteredFolders;
    std::vector<size_t> filteredFileIndices;
    
    if (m_searchQuery.empty()) 
    {
        filteredFolders = m_currentFolders;
        for (size_t i = 0; i < m_currentFiles.size(); i++) 
        {
            filteredFileIndices.push_back(i);
        }
    }
    else 
    {
        std::string query = m_searchQuery;
        std::transform(query.begin(), query.end(), query.begin(), ::tolower);
        
        // Lọc folder
        for (const auto& folder : m_currentFolders) 
        {
            std::string lowerName = folder.name;
            std::transform(lowerName.begin(), lowerName.end(), lowerName.begin(), ::tolower);
            if (lowerName.find(query) != std::string::npos) 
            {
                filteredFolders.push_back(folder);
            }
        }
        
        // Lọc file
        for (size_t i = 0; i < m_currentFiles.size(); i++) 
        {
            const auto& media = m_currentFiles[i];
            std::string title = media.getTitle().empty() ? media.getFileName() : media.getTitle();
            std::transform(title.begin(), title.end(), title.begin(), ::tolower);
            
            std::string artist = media.getArtist();
            std::transform(artist.begin(), artist.end(), artist.begin(), ::tolower);
            
            if (title.find(query) != std::string::npos || 
                artist.find(query) != std::string::npos) 
            {
                filteredFileIndices.push_back(i);
            }
        }
    }
    
    // Render folder list
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
        
        // Click handler — navigate vào folder
        if (!inputBlocked && hover 
            && painter.isLeftMouseClicked(x, itemY, w, itemH)) 
        {
            navigateToFolder(folder.fullPath);
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
    
    // Render file list
    for (size_t fi = 0; fi < filteredFileIndices.size(); fi++) 
    {
        size_t fileIdx = filteredFileIndices[fi];
        const auto& media = m_currentFiles[fileIdx];
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
        
        // Click handler
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
                // Mở context menu
                m_showContextMenu = true;
                m_contextMenuX = mx;
                m_contextMenuY = my;
                m_contextMenuIndex = static_cast<int>(fileIdx);
                painter.consumeClick();
            }
            else if (leftClickRow) 
            {
                // Play Now — giống logic LibraryScreen
                if (!media.isUnsupported() && m_playbackController && m_queueController) 
                {
                    // Kiểm tra bài đã có trong queue chưa
                    int existingIndex = -1;
                    auto queueItems = m_queueController->getAllItems();
                    for (size_t qi = 0; qi < queueItems.size(); ++qi) 
                    {
                        if (queueItems[qi].getFilePath() == media.getFilePath()) 
                        {
                            existingIndex = static_cast<int>(qi);
                            break;
                        }
                    }
                    
                    if (existingIndex >= 0) 
                    {
                        // Bài đã trong queue — nhảy tới và play
                        m_queueController->jumpToIndex(static_cast<size_t>(existingIndex));
                        m_playbackController->play();
                    }
                    else if (m_queueController->isEmpty()) 
                    {
                        m_queueController->addToQueue(media);
                        m_playbackController->play();
                    }
                    else 
                    {
                        // Thêm bài tiếp theo và play
                        size_t nextIdx = m_queueController->getCurrentIndex() + 1;
                        m_queueController->addToQueueNext(media);
                        m_queueController->jumpToIndex(nextIdx);
                        m_playbackController->play();
                    }
                }
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
// Breadcrumb
// ============================================================================

void ExploreScreen::renderBreadcrumb(ui::ImGuiManager& painter, int x, int y, int w) 
{
    const auto& theme = painter.getTheme();
    
    // Background bar
    painter.drawRect(x, y, w, 28, theme.surface);
    
    int textX = x + 10;
    
    // Back button (nếu không ở root)
    if (m_currentPath != m_rootPath && !m_currentPath.empty()) 
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
            navigateUp();
            painter.consumeClick();
            return;
        }
        
        textX += backBtnW + 10;
    }
    
    // Tạo breadcrumb segments từ path
    // Root: /media/duong/Music
    // Current: /media/duong/Music/Rock/Album1
    // Hiển thị: Root > Rock > Album1
    
    if (m_currentPath.length() >= m_rootPath.length()) 
    {
        // Root segment
        std::string rootName = fs::path(m_rootPath).filename().string();
        if (rootName.empty()) 
        {
            rootName = m_rootPath;
        }
        
        bool isRoot = (m_currentPath == m_rootPath);
        uint32_t rootColor = isRoot ? theme.textPrimary : theme.primary;
        
        // Click root → navigate về root
        int rootW = static_cast<int>(rootName.length()) * 8 + 10;
        bool rootHover = !isRoot && painter.isMouseOver(textX, y, rootW, 28);
        
        painter.drawText(rootName, textX, y + 6, 
                         rootHover ? theme.primaryHover : rootColor, 13);
        
        if (rootHover && painter.isMouseClicked(textX, y, rootW, 28)) 
        {
            m_currentPath = m_rootPath;
            m_pathStack.clear();
            m_scrollOffset = 0;
            buildCurrentView();
            painter.consumeClick();
            return;
        }
        
        textX += rootW;
        
        // Sub-segments (nếu đang trong subfolder)
        if (!isRoot) 
        {
            std::string relative = m_currentPath.substr(m_rootPath.length());
            if (!relative.empty() && relative[0] == '/') 
            {
                relative = relative.substr(1);
            }
            
            std::istringstream iss(relative);
            std::string segment;
            std::string builtPath = m_rootPath;
            
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
                
                bool isLast = (builtPath == m_currentPath);
                int segW = static_cast<int>(segment.length()) * 8 + 10;
                
                // Giới hạn breadcrumb không tràn quá width
                if (textX + segW > x + w - 20) 
                {
                    painter.drawText("...", textX, y + 6, theme.textDim, 13);
                    break;
                }
                
                // Click segment → navigate tới folder đó
                bool segHover = !isLast && painter.isMouseOver(textX, y, segW, 28);
                uint32_t segColor = isLast ? theme.textPrimary : 
                                    (segHover ? theme.primaryHover : theme.primary);
                
                painter.drawText(segment, textX, y + 6, segColor, 13);
                
                if (segHover && painter.isMouseClicked(textX, y, segW, 28)) 
                {
                    m_currentPath = builtPath;
                    // Xóa path stack về đúng vị trí
                    m_pathStack.clear();
                    m_scrollOffset = 0;
                    buildCurrentView();
                    painter.consumeClick();
                    return;
                }
                
                textX += segW;
            }
        }
    }
}

// ============================================================================
// Context Menu (giống LibraryScreen)
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
                if (m_contextMenuIndex >= 0 
                    && m_contextMenuIndex < static_cast<int>(m_currentFiles.size())) 
                {
                    const auto& targetMedia = m_currentFiles[m_contextMenuIndex];
                    
                    if (i == 0) 
                    {
                        // Add to Queue
                        if (m_queueController) 
                        {
                            m_queueController->addToQueue(targetMedia);
                        }
                    }
                    else if (i == 1) 
                    {
                        // Play Next
                        if (m_queueController) 
                        {
                            m_queueController->addToQueueNext(targetMedia);
                        }
                    }
                    else if (i == 2) 
                    {
                        // Add to Playlist
                        if (m_playlistController) 
                        {
                            auto& state = painter.getState();
                            state.showAddToPlaylistDialog = true;
                            state.contextMediaItem = targetMedia;
                        }
                    }
                    else if (i == 3) 
                    {
                        // Properties
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
                        int dur = targetMedia.getDuration();
                        state.metadataEdit.durationStr = (dur > 0) ? 
                            (std::to_string(dur / 60) + ":" + (dur % 60 < 10 ? "0" : "") 
                             + std::to_string(dur % 60)) : "-";
                        
                        // Metadata từ MediaFileModel
                        state.metadataEdit.title = targetMedia.getTitle().empty() ? 
                            targetMedia.getFileName() : targetMedia.getTitle();
                        state.metadataEdit.artist = targetMedia.getArtist().empty() ? 
                            "-" : targetMedia.getArtist();
                        state.metadataEdit.album = targetMedia.getAlbum().empty() ? 
                            "-" : targetMedia.getAlbum();
                        state.metadataEdit.genre = "-";
                        state.metadataEdit.year = "-";
                        state.metadataEdit.publisher = "-";
                        state.metadataEdit.bitrateStr = "-";
                        
                        // Đọc metadata chi tiết từ LibraryController
                        if (m_libraryController && !targetMedia.isUnsupported()) 
                        {
                            auto meta = m_libraryController->readMetadata(targetMedia.getFilePath());
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
        
        // Tính max scroll
        int totalItems = static_cast<int>(m_currentFolders.size()) * 45 
                       + static_cast<int>(m_currentFiles.size()) * 50;
        int listH = 500; // Ước lượng chiều cao visible
        int maxScroll = std::max(0, totalItems - listH);
        
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
