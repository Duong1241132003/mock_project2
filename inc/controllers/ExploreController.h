#ifndef EXPLORE_CONTROLLER_H
#define EXPLORE_CONTROLLER_H

// System includes
#include <memory>
#include <vector>
#include <string>

// Project includes
#include "models/ExploreModel.h"
#include "models/MediaFileModel.h"

// Forward declarations
namespace media_player 
{
namespace controllers 
{
    class LibraryController;
    class QueueController;
    class PlaybackController;
    class PlaylistController;
}
}

namespace media_player 
{
namespace controllers 
{

/**
 * @brief Controller cho tính năng Explore — duyệt thư viện theo cấu trúc thư mục.
 * 
 * Theo chuẩn MVC, Controller xử lý toàn bộ business logic:
 * - Navigation giữa các folder
 * - Xây dựng danh sách folder/file từ media list
 * - Lọc theo search query
 * - Phối hợp với Queue/Playback controllers cho playback actions
 * 
 * Controller đọc/ghi dữ liệu qua ExploreModel, 
 * lấy media data từ LibraryController.
 */
class ExploreController 
{
public:
    /**
     * @brief Constructs ExploreController với các dependencies cần thiết.
     * @param exploreModel Model lưu trữ state
     * @param libraryController Để lấy danh sách media
     * @param queueController Để thêm bài vào queue
     * @param playbackController Để điều khiển phát nhạc
     * @param playlistController Để thêm bài vào playlist
     */
    ExploreController(
        std::shared_ptr<models::ExploreModel> exploreModel,
        std::shared_ptr<LibraryController> libraryController,
        std::shared_ptr<QueueController> queueController,
        std::shared_ptr<PlaybackController> playbackController,
        std::shared_ptr<PlaylistController> playlistController
    );
    
    ~ExploreController();
    
    // ==================== Navigation ====================
    
    /**
     * @brief Navigate vào một folder cụ thể.
     * Lưu path hiện tại vào stack, cập nhật model.
     */
    void navigateToFolder(const std::string& folderPath);
    
    /**
     * @brief Quay lại folder trước đó (pop stack).
     */
    void navigateUp();
    
    /**
     * @brief Navigate về root folder.
     */
    void navigateToRoot();
    
    /**
     * @brief Navigate tới một segment cụ thể trong breadcrumb.
     */
    void navigateToBreadcrumb(const std::string& path);
    
    // ==================== Data Loading ====================
    
    /**
     * @brief Set root path và tải media list từ LibraryController.
     * Được gọi sau khi scan hoàn tất.
     */
    void setRootPath(const std::string& rootPath);
    
    /**
     * @brief Tải lại media list từ LibraryController.
     */
    void refreshMediaList();
    
    // ==================== View Data Access ====================
    
    /**
     * @brief Lấy danh sách folder cho view hiển thị.
     * Có thể đã lọc theo search query.
     */
    std::vector<models::FolderEntry> getFilteredFolders(const std::string& searchQuery) const;
    
    /**
     * @brief Lấy danh sách file cho view hiển thị (trả về indices).
     * Có thể đã lọc theo search query.
     */
    std::vector<size_t> getFilteredFileIndices(const std::string& searchQuery) const;
    
    /**
     * @brief Lấy thông tin folder/file count hiện tại.
     */
    size_t getFolderCount() const;
    size_t getFileCount() const;
    
    /**
     * @brief Lấy current path, root path, kiểm tra vị trí.
     */
    std::string getCurrentPath() const;
    std::string getRootPath() const;
    bool isAtRoot() const;
    
    /**
     * @brief Lấy file tại index cụ thể.
     */
    const models::MediaFileModel* getFileAt(size_t index) const;
    
    // ==================== Playback Actions ====================
    
    /**
     * @brief Phát file tại index trong danh sách hiện tại.
     * Coordinate với Queue + Playback controllers.
     */
    void playFile(size_t fileIndex);
    
    /**
     * @brief Thêm file vào cuối queue.
     */
    void addToQueue(size_t fileIndex);
    
    /**
     * @brief Thêm file vào vị trí tiếp theo trong queue.
     */
    void addToQueueNext(size_t fileIndex);
    
    // ==================== Access to Controllers ====================
    
    /**
     * @brief Lấy PlaylistController (để view mở dialog Add to Playlist).
     */
    std::shared_ptr<PlaylistController> getPlaylistController() const;
    
    /**
     * @brief Lấy LibraryController (để view đọc metadata Properties).
     */
    std::shared_ptr<LibraryController> getLibraryController() const;

private:
    /**
     * @brief Xây dựng danh sách folder + file từ allMedia theo path prefix.
     * Cập nhật model với kết quả.
     */
    void buildCurrentView();
    
    std::shared_ptr<models::ExploreModel> m_exploreModel;         ///< Data model
    std::shared_ptr<LibraryController> m_libraryController;       ///< Media data source
    std::shared_ptr<QueueController> m_queueController;           ///< Queue management
    std::shared_ptr<PlaybackController> m_playbackController;     ///< Playback control
    std::shared_ptr<PlaylistController> m_playlistController;     ///< Playlist management
};

} // namespace controllers
} // namespace media_player

#endif // EXPLORE_CONTROLLER_H
