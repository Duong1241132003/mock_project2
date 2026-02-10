// Project includes
#include "controllers/ExploreController.h"
#include "controllers/LibraryController.h"
#include "controllers/QueueController.h"
#include "controllers/PlaybackController.h"
#include "controllers/PlaylistController.h"

// System includes
#include <algorithm>
#include <filesystem>
#include <set>

namespace fs = std::filesystem;

namespace media_player 
{
namespace controllers 
{

// ============================================================================
// Constructor / Destructor
// ============================================================================

ExploreController::ExploreController(
    std::shared_ptr<models::ExploreModel> exploreModel,
    std::shared_ptr<LibraryController> libraryController,
    std::shared_ptr<QueueController> queueController,
    std::shared_ptr<PlaybackController> playbackController,
    std::shared_ptr<PlaylistController> playlistController
)
    : m_exploreModel(exploreModel)
    , m_libraryController(libraryController)
    , m_queueController(queueController)
    , m_playbackController(playbackController)
    , m_playlistController(playlistController)
{
}

ExploreController::~ExploreController() 
{
}

// ============================================================================
// Navigation
// ============================================================================

void ExploreController::navigateToFolder(const std::string& folderPath) 
{
    // Lưu path hiện tại vào stack để hỗ trợ Back
    m_exploreModel->pushPath(m_exploreModel->getCurrentPath());
    m_exploreModel->setCurrentPath(folderPath);
    
    buildCurrentView();
}

void ExploreController::navigateUp() 
{
    if (!m_exploreModel->isPathStackEmpty()) 
    {
        // Quay lại path trước đó từ stack
        std::string previousPath = m_exploreModel->popPath();
        m_exploreModel->setCurrentPath(previousPath);
    }
    else if (!m_exploreModel->isAtRoot()) 
    {
        // Fallback: navigate lên parent directory
        fs::path parentPath = fs::path(m_exploreModel->getCurrentPath()).parent_path();
        
        // Không navigate ra ngoài root
        if (parentPath.string().length() >= m_exploreModel->getRootPath().length()) 
        {
            m_exploreModel->setCurrentPath(parentPath.string());
        }
    }
    
    buildCurrentView();
}

void ExploreController::navigateToRoot() 
{
    m_exploreModel->setCurrentPath(m_exploreModel->getRootPath());
    m_exploreModel->clearPathStack();
    
    buildCurrentView();
}

void ExploreController::navigateToBreadcrumb(const std::string& path) 
{
    m_exploreModel->setCurrentPath(path);
    // Xóa path stack khi navigate qua breadcrumb
    m_exploreModel->clearPathStack();
    
    buildCurrentView();
}

// ============================================================================
// Data Loading
// ============================================================================

void ExploreController::setRootPath(const std::string& rootPath) 
{
    m_exploreModel->setRootPath(rootPath);
    m_exploreModel->setCurrentPath(rootPath);
    m_exploreModel->clearPathStack();
    
    // Tải media list từ LibraryController
    refreshMediaList();
    
    buildCurrentView();
}

void ExploreController::refreshMediaList() 
{
    if (m_libraryController) 
    {
        m_exploreModel->setAllMedia(m_libraryController->getAllMedia());
    }
    
    // Nếu chưa có current path, dùng root
    if (m_exploreModel->getCurrentPath().empty() 
        && !m_exploreModel->getRootPath().empty()) 
    {
        m_exploreModel->setCurrentPath(m_exploreModel->getRootPath());
    }
    
    buildCurrentView();
}

// ============================================================================
// View Data Access
// ============================================================================

std::vector<models::FolderEntry> ExploreController::getFilteredFolders(
    const std::string& searchQuery) const 
{
    const auto& allFolders = m_exploreModel->getCurrentFolders();
    
    if (searchQuery.empty()) 
    {
        return allFolders;
    }
    
    // Chuyển query sang lowercase để so sánh case-insensitive
    std::string query = searchQuery;
    std::transform(query.begin(), query.end(), query.begin(), ::tolower);
    
    std::vector<models::FolderEntry> filtered;
    for (const auto& folder : allFolders) 
    {
        std::string lowerName = folder.name;
        std::transform(lowerName.begin(), lowerName.end(), lowerName.begin(), ::tolower);
        
        if (lowerName.find(query) != std::string::npos) 
        {
            filtered.push_back(folder);
        }
    }
    
    return filtered;
}

std::vector<size_t> ExploreController::getFilteredFileIndices(
    const std::string& searchQuery) const 
{
    const auto& allFiles = m_exploreModel->getCurrentFiles();
    std::vector<size_t> indices;
    
    if (searchQuery.empty()) 
    {
        // Trả về tất cả indices
        for (size_t i = 0; i < allFiles.size(); i++) 
        {
            indices.push_back(i);
        }
        return indices;
    }
    
    // Lọc theo query (title hoặc artist)
    std::string query = searchQuery;
    std::transform(query.begin(), query.end(), query.begin(), ::tolower);
    
    for (size_t i = 0; i < allFiles.size(); i++) 
    {
        const auto& media = allFiles[i];
        
        std::string title = media.getTitle().empty() ? media.getFileName() : media.getTitle();
        std::transform(title.begin(), title.end(), title.begin(), ::tolower);
        
        std::string artist = media.getArtist();
        std::transform(artist.begin(), artist.end(), artist.begin(), ::tolower);
        
        if (title.find(query) != std::string::npos || 
            artist.find(query) != std::string::npos) 
        {
            indices.push_back(i);
        }
    }
    
    return indices;
}

size_t ExploreController::getFolderCount() const 
{
    return m_exploreModel->getFolderCount();
}

size_t ExploreController::getFileCount() const 
{
    return m_exploreModel->getFileCount();
}

std::string ExploreController::getCurrentPath() const 
{
    return m_exploreModel->getCurrentPath();
}

std::string ExploreController::getRootPath() const 
{
    return m_exploreModel->getRootPath();
}

bool ExploreController::isAtRoot() const 
{
    return m_exploreModel->isAtRoot();
}

const models::MediaFileModel* ExploreController::getFileAt(size_t index) const 
{
    return m_exploreModel->getFileAt(index);
}

// ============================================================================
// Playback Actions
// ============================================================================

void ExploreController::playFile(size_t fileIndex) 
{
    const auto* media = m_exploreModel->getFileAt(fileIndex);
    if (!media || media->isUnsupported() || !m_playbackController || !m_queueController) 
    {
        return;
    }
    
    // Kiểm tra bài đã có trong queue chưa
    int existingIndex = -1;
    auto queueItems = m_queueController->getAllItems();
    
    for (size_t qi = 0; qi < queueItems.size(); ++qi) 
    {
        if (queueItems[qi].getFilePath() == media->getFilePath()) 
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
        // Queue trống — thêm và play
        m_queueController->addToQueue(*media);
        m_playbackController->play();
    }
    else 
    {
        // Thêm bài tiếp theo, nhảy tới và play
        size_t nextIdx = m_queueController->getCurrentIndex() + 1;
        m_queueController->addToQueueNext(*media);
        m_queueController->jumpToIndex(nextIdx);
        m_playbackController->play();
    }
}

void ExploreController::addToQueue(size_t fileIndex) 
{
    const auto* media = m_exploreModel->getFileAt(fileIndex);
    if (media && m_queueController) 
    {
        m_queueController->addToQueue(*media);
    }
}

void ExploreController::addToQueueNext(size_t fileIndex) 
{
    const auto* media = m_exploreModel->getFileAt(fileIndex);
    if (media && m_queueController) 
    {
        m_queueController->addToQueueNext(*media);
    }
}

// ============================================================================
// Access to Controllers
// ============================================================================

std::shared_ptr<PlaylistController> ExploreController::getPlaylistController() const 
{
    return m_playlistController;
}

std::shared_ptr<LibraryController> ExploreController::getLibraryController() const 
{
    return m_libraryController;
}

// ============================================================================
// Private: Xây dựng danh sách folder + file từ media list
// ============================================================================

void ExploreController::buildCurrentView() 
{
    std::vector<models::FolderEntry> folders;
    std::vector<models::MediaFileModel> files;
    
    std::string currentPath = m_exploreModel->getCurrentPath();
    if (currentPath.empty()) 
    {
        m_exploreModel->setCurrentFolders(folders);
        m_exploreModel->setCurrentFiles(files);
        return;
    }
    
    // Đảm bảo currentPath kết thúc bằng '/'
    std::string prefix = currentPath;
    if (!prefix.empty() && prefix.back() != '/') 
    {
        prefix += '/';
    }
    
    // Dùng set để thu thập tên subfolder (không trùng lặp)
    std::set<std::string> subfolderNames;
    const auto& allMedia = m_exploreModel->getAllMedia();
    
    for (const auto& media : allMedia) 
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
            files.push_back(media);
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
        models::FolderEntry entry;
        entry.name = name;
        entry.fullPath = prefix + name;
        
        // Đếm số file trong subfolder (đệ quy)
        std::string subPrefix = entry.fullPath + "/";
        entry.fileCount = 0;
        
        for (const auto& media : allMedia) 
        {
            if (media.getFilePath().compare(0, subPrefix.length(), subPrefix) == 0) 
            {
                entry.fileCount++;
            }
        }
        
        folders.push_back(entry);
    }
    
    // Sắp xếp folder theo tên (alphabetical)
    std::sort(folders.begin(), folders.end(),
        [](const models::FolderEntry& a, const models::FolderEntry& b) 
        {
            return a.name < b.name;
        });
    
    // Sắp xếp file theo tên
    std::sort(files.begin(), files.end(),
        [](const models::MediaFileModel& a, const models::MediaFileModel& b) 
        {
            std::string nameA = a.getTitle().empty() ? a.getFileName() : a.getTitle();
            std::string nameB = b.getTitle().empty() ? b.getFileName() : b.getTitle();
            return nameA < nameB;
        });
    
    // Cập nhật model
    m_exploreModel->setCurrentFolders(folders);
    m_exploreModel->setCurrentFiles(files);
}

} // namespace controllers
} // namespace media_player
