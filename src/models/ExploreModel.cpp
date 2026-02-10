// Project includes
#include "models/ExploreModel.h"

namespace media_player 
{
namespace models 
{

// ============================================================================
// Root Path
// ============================================================================

void ExploreModel::setRootPath(const std::string& rootPath) 
{
    m_rootPath = rootPath;
}

std::string ExploreModel::getRootPath() const 
{
    return m_rootPath;
}

// ============================================================================
// Current Path
// ============================================================================

void ExploreModel::setCurrentPath(const std::string& path) 
{
    m_currentPath = path;
}

std::string ExploreModel::getCurrentPath() const 
{
    return m_currentPath;
}

bool ExploreModel::isAtRoot() const 
{
    return m_currentPath == m_rootPath || m_currentPath.empty();
}

// ============================================================================
// Path Stack
// ============================================================================

void ExploreModel::pushPath(const std::string& path) 
{
    m_pathStack.push_back(path);
}

std::string ExploreModel::popPath() 
{
    if (m_pathStack.empty()) 
    {
        return "";
    }
    
    std::string path = m_pathStack.back();
    m_pathStack.pop_back();
    return path;
}

bool ExploreModel::isPathStackEmpty() const 
{
    return m_pathStack.empty();
}

void ExploreModel::clearPathStack() 
{
    m_pathStack.clear();
}

// ============================================================================
// View Data
// ============================================================================

void ExploreModel::setCurrentFolders(const std::vector<FolderEntry>& folders) 
{
    m_currentFolders = folders;
}

const std::vector<FolderEntry>& ExploreModel::getCurrentFolders() const 
{
    return m_currentFolders;
}

void ExploreModel::setCurrentFiles(const std::vector<MediaFileModel>& files) 
{
    m_currentFiles = files;
}

const std::vector<MediaFileModel>& ExploreModel::getCurrentFiles() const 
{
    return m_currentFiles;
}

const MediaFileModel* ExploreModel::getFileAt(size_t index) const 
{
    if (index < m_currentFiles.size()) 
    {
        return &m_currentFiles[index];
    }
    return nullptr;
}

// ============================================================================
// All Media Cache
// ============================================================================

void ExploreModel::setAllMedia(const std::vector<MediaFileModel>& allMedia) 
{
    m_allMedia = allMedia;
}

const std::vector<MediaFileModel>& ExploreModel::getAllMedia() const 
{
    return m_allMedia;
}

// ============================================================================
// Statistics
// ============================================================================

size_t ExploreModel::getFolderCount() const 
{
    return m_currentFolders.size();
}

size_t ExploreModel::getFileCount() const 
{
    return m_currentFiles.size();
}

} // namespace models
} // namespace media_player
