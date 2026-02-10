#ifndef EXPLORE_MODEL_H
#define EXPLORE_MODEL_H

// System includes
#include <string>
#include <vector>

// Project includes
#include "MediaFileModel.h"

namespace media_player 
{
namespace models 
{

/**
 * @brief Cấu trúc đại diện cho một folder entry trong danh sách hiển thị.
 */
struct FolderEntry
{
    std::string name;      ///< Tên folder (chỉ phần cuối)
    std::string fullPath;  ///< Đường dẫn đầy đủ tới folder
    int fileCount;         ///< Số file media bên trong (đệ quy)
};

/**
 * @brief Model cho tính năng Explore — lưu trữ dữ liệu duyệt thư mục.
 * 
 * Theo chuẩn MVC, Model chỉ lưu trữ và cung cấp dữ liệu.
 * Mọi business logic (navigate, filter, build view) nằm ở ExploreController.
 */
class ExploreModel 
{
public:
    ExploreModel() = default;
    
    // ==================== Root Path ====================
    
    /**
     * @brief Set đường dẫn gốc (folder đã quét).
     */
    void setRootPath(const std::string& rootPath);
    
    /**
     * @brief Lấy đường dẫn gốc.
     */
    std::string getRootPath() const;
    
    // ==================== Current Path ====================
    
    /**
     * @brief Set đường dẫn folder đang xem.
     */
    void setCurrentPath(const std::string& path);
    
    /**
     * @brief Lấy đường dẫn folder đang xem.
     */
    std::string getCurrentPath() const;
    
    /**
     * @brief Kiểm tra có đang ở root không.
     */
    bool isAtRoot() const;
    
    // ==================== Path Stack (Navigation History) ====================
    
    /**
     * @brief Đẩy path hiện tại vào stack trước khi navigate.
     */
    void pushPath(const std::string& path);
    
    /**
     * @brief Lấy path cuối cùng từ stack (và xóa khỏi stack).
     * @return Path trước đó, hoặc chuỗi rỗng nếu stack trống.
     */
    std::string popPath();
    
    /**
     * @brief Kiểm tra stack có rỗng không.
     */
    bool isPathStackEmpty() const;
    
    /**
     * @brief Xóa toàn bộ path stack.
     */
    void clearPathStack();
    
    // ==================== View Data ====================
    
    /**
     * @brief Cập nhật danh sách subfolder cho folder hiện tại.
     */
    void setCurrentFolders(const std::vector<FolderEntry>& folders);
    
    /**
     * @brief Lấy danh sách subfolder hiện tại.
     */
    const std::vector<FolderEntry>& getCurrentFolders() const;
    
    /**
     * @brief Cập nhật danh sách file media cho folder hiện tại.
     */
    void setCurrentFiles(const std::vector<MediaFileModel>& files);
    
    /**
     * @brief Lấy danh sách file media hiện tại.
     */
    const std::vector<MediaFileModel>& getCurrentFiles() const;
    
    /**
     * @brief Lấy file tại index cụ thể.
     * @return Pointer tới file, nullptr nếu index không hợp lệ.
     */
    const MediaFileModel* getFileAt(size_t index) const;
    
    // ==================== All Media Cache ====================
    
    /**
     * @brief Cập nhật cache toàn bộ media list.
     */
    void setAllMedia(const std::vector<MediaFileModel>& allMedia);
    
    /**
     * @brief Lấy toàn bộ media list (đã cache).
     */
    const std::vector<MediaFileModel>& getAllMedia() const;
    
    // ==================== Statistics ====================
    
    /**
     * @brief Lấy tổng số folder hiện tại.
     */
    size_t getFolderCount() const;
    
    /**
     * @brief Lấy tổng số file hiện tại.
     */
    size_t getFileCount() const;
    
private:
    std::string m_rootPath;            ///< Đường dẫn gốc (scan root)
    std::string m_currentPath;         ///< Folder đang xem
    std::vector<std::string> m_pathStack; ///< Lịch sử navigation
    
    std::vector<FolderEntry> m_currentFolders;       ///< Subfolder hiện tại
    std::vector<MediaFileModel> m_currentFiles;      ///< File nhạc hiện tại
    std::vector<MediaFileModel> m_allMedia;          ///< Cache toàn bộ media
};

} // namespace models
} // namespace media_player

#endif // EXPLORE_MODEL_H
