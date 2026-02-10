#ifndef EXPLORE_SCREEN_H
#define EXPLORE_SCREEN_H

// System includes
#include <memory>
#include <vector>
#include <string>
#include <stack>

// Project includes
#include "IView.h"
#include "controllers/LibraryController.h"
#include "controllers/QueueController.h"
#include "controllers/PlaybackController.h"
#include "controllers/PlaylistController.h"
#include "models/MediaFileModel.h"

namespace media_player 
{
namespace views 
{

/**
 * @brief Màn hình Explore — hiển thị thư viện nhạc theo cấu trúc thư mục.
 * 
 * Cho phép người dùng duyệt qua các folder đã quét, click vào folder 
 * để xem nội dung (subfolder + file nhạc bên trong).
 */
class ExploreScreen : public IView 
{
public:
    ExploreScreen(
        std::shared_ptr<controllers::LibraryController> libraryController,
        std::shared_ptr<controllers::QueueController> queueController,
        std::shared_ptr<controllers::PlaybackController> playbackController,
        std::shared_ptr<controllers::PlaylistController> playlistController
    );
    
    ~ExploreScreen();
    
    // IView interface
    void show() override;
    void hide() override;
    void update() override;
    bool isVisible() const override;
    
    // ImGui View Interface
    void render(ui::ImGuiManager& painter) override;
    bool handleInput(const SDL_Event& event) override;
    
    /**
     * @brief Cập nhật root path khi scan hoàn tất.
     * @param rootPath Đường dẫn gốc (folder đã quét)
     */
    void setRootPath(const std::string& rootPath);
    
private:
    // Cấu trúc đại diện cho một folder entry trong danh sách hiển thị
    struct FolderEntry
    {
        std::string name;      // Tên folder
        std::string fullPath;  // Đường dẫn đầy đủ
        int fileCount;         // Số file media bên trong (đệ quy)
    };
    
    /**
     * @brief Navigate vào folder cụ thể.
     * Cập nhật danh sách subfolder và file trong folder đó.
     */
    void navigateToFolder(const std::string& folderPath);
    
    /**
     * @brief Quay lại folder cha.
     */
    void navigateUp();
    
    /**
     * @brief Xây dựng danh sách subfolder và file từ media list.
     * Lọc từ LibraryController::getAllMedia() theo path prefix.
     */
    void buildCurrentView();
    
    /**
     * @brief Render breadcrumb navigation bar.
     */
    void renderBreadcrumb(ui::ImGuiManager& painter, int x, int y, int w);
    
    /**
     * @brief Render danh sách folder.
     */
    void renderFolderList(ui::ImGuiManager& painter, int x, int& y, int w, int listH, bool inputBlocked);
    
    /**
     * @brief Render danh sách file media.
     */
    void renderFileList(ui::ImGuiManager& painter, int x, int& y, int w, int listH, bool inputBlocked);
    
    /**
     * @brief Render context menu cho file (giống LibraryScreen).
     */
    void renderContextMenu(ui::ImGuiManager& painter);
    
    // State
    bool m_isVisible;
    std::string m_rootPath;           // Đường dẫn gốc (scan root)
    std::string m_currentPath;        // Folder đang xem hiện tại
    std::vector<std::string> m_pathStack; // Lịch sử navigation để back
    
    // Dữ liệu hiển thị cho folder hiện tại
    std::vector<FolderEntry> m_currentFolders;  // Subfolder
    std::vector<models::MediaFileModel> m_currentFiles; // File nhạc
    
    // Toàn bộ media list (cache từ controller)
    std::vector<models::MediaFileModel> m_allMedia;
    
    // Scroll state
    int m_scrollOffset;
    
    // Search state (đồng bộ với global search)
    std::string m_searchQuery;
    
    // Context Menu State
    bool m_showContextMenu;
    int m_contextMenuX;
    int m_contextMenuY;
    int m_contextMenuIndex; // Index trong m_currentFiles
    
    // Dependencies
    std::shared_ptr<controllers::LibraryController> m_libraryController;
    std::shared_ptr<controllers::QueueController> m_queueController;
    std::shared_ptr<controllers::PlaybackController> m_playbackController;
    std::shared_ptr<controllers::PlaylistController> m_playlistController;
};

} // namespace views
} // namespace media_player

#endif // EXPLORE_SCREEN_H
