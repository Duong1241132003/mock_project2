// Project includes
#include "views/LibraryScreen.h"
#include "config/AppConfig.h"

namespace media_player 
{
namespace views 
{

LibraryScreen::LibraryScreen(
    std::shared_ptr<controllers::LibraryController> libraryController,
    std::shared_ptr<controllers::QueueController> queueController
)
    : m_libraryController(libraryController)
    , m_queueController(queueController)
    , m_currentPage(0)
    , m_itemsPerPage(config::AppConfig::MAX_ITEMS_PER_PAGE)
    , m_selectedIndex(-1)
    , m_isVisible(false)
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
    if (!m_isVisible) 
    {
        return;
    }
    
    displayMediaList();
}

bool LibraryScreen::isVisible() const 
{
    return m_isVisible;
}

void LibraryScreen::nextPage() 
{
    size_t totalItems = m_currentMediaList.size();
    size_t totalPages = (totalItems + m_itemsPerPage - 1) / m_itemsPerPage;
    
    if (m_currentPage < totalPages - 1) 
    {
        m_currentPage++;
        update();
    }
}

void LibraryScreen::previousPage() 
{
    if (m_currentPage > 0) 
    {
        m_currentPage--;
        update();
    }
}

void LibraryScreen::goToPage(size_t pageNumber) 
{
    size_t totalItems = m_currentMediaList.size();
    size_t totalPages = (totalItems + m_itemsPerPage - 1) / m_itemsPerPage;
    
    if (pageNumber < totalPages) 
    {
        m_currentPage = pageNumber;
        update();
    }
}

void LibraryScreen::showAllMedia() 
{
    m_currentMediaList = m_libraryController->getAllMedia();
    m_currentPage = 0;
    update();
}

void LibraryScreen::showAudioOnly() 
{
    m_currentMediaList = m_libraryController->getAudioFiles();
    m_currentPage = 0;
    update();
}

void LibraryScreen::showVideoOnly() 
{
    m_currentMediaList = m_libraryController->getVideoFiles();
    m_currentPage = 0;
    update();
}

void LibraryScreen::searchMedia(const std::string& query) 
{
    m_currentMediaList = m_libraryController->search(query);
    m_currentPage = 0;
    update();
}

void LibraryScreen::sortByTitle() 
{
    m_currentMediaList = m_libraryController->sortByTitle();
    update();
}

void LibraryScreen::sortByArtist() 
{
    m_currentMediaList = m_libraryController->sortByArtist();
    update();
}

void LibraryScreen::sortByAlbum() 
{
    m_currentMediaList = m_libraryController->sortByAlbum();
    update();
}

void LibraryScreen::playSelected() 
{
    if (m_selectedIndex < 0 || m_selectedIndex >= static_cast<int>(m_currentMediaList.size())) 
    {
        return;
    }
    
    m_queueController->clearQueue();
    m_queueController->addToQueue(m_currentMediaList[m_selectedIndex]);
    
    // Trigger playback
}

void LibraryScreen::addSelectedToQueue() 
{
    if (m_selectedIndex < 0 || m_selectedIndex >= static_cast<int>(m_currentMediaList.size())) 
    {
        return;
    }
    
    m_queueController->addToQueue(m_currentMediaList[m_selectedIndex]);
}

void LibraryScreen::addAllToQueue() 
{
    for (const auto& media : m_currentMediaList) 
    {
        m_queueController->addToQueue(media);
    }
}

void LibraryScreen::refreshMediaList() 
{
    m_currentMediaList = m_libraryController->getAllMedia();
    m_currentPage = 0;
}

void LibraryScreen::displayMediaList() 
{
    size_t startIndex = m_currentPage * m_itemsPerPage;
    size_t endIndex = std::min(startIndex + m_itemsPerPage, m_currentMediaList.size());
    
    for (size_t i = startIndex; i < endIndex; ++i) 
    {
        const auto& media = m_currentMediaList[i];
    }
}

} // namespace views
} // namespace media_player
