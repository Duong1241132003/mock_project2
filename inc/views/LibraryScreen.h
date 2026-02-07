#ifndef LIBRARY_SCREEN_H
#define LIBRARY_SCREEN_H

// System includes
#include <memory>
#include <vector>

// Project includes
#include "IView.h"
#include "controllers/LibraryController.h"
#include "controllers/QueueController.h"
#include "models/MediaFileModel.h"

namespace media_player 
{
namespace views 
{

class LibraryScreen : public IView 
{
public:
    LibraryScreen(
        std::shared_ptr<controllers::LibraryController> libraryController,
        std::shared_ptr<controllers::QueueController> queueController
    );
    
    ~LibraryScreen();
    
    // IView interface
    void show() override;
    void hide() override;
    void update() override;
    bool isVisible() const override;
    
    // Navigation
    void nextPage();
    void previousPage();
    void goToPage(size_t pageNumber);
    
    // Filtering
    void showAllMedia();
    void showAudioOnly();
    void showVideoOnly();
    void searchMedia(const std::string& query);
    
    // Sorting
    void sortByTitle();
    void sortByArtist();
    void sortByAlbum();
    
    // Actions
    void playSelected();
    void addSelectedToQueue();
    void addAllToQueue();
    
private:
    void refreshMediaList();
    void displayMediaList();
    
    std::shared_ptr<controllers::LibraryController> m_libraryController;
    std::shared_ptr<controllers::QueueController> m_queueController;
    
    std::vector<models::MediaFileModel> m_currentMediaList;
    size_t m_currentPage;
    size_t m_itemsPerPage;
    int m_selectedIndex;
    
    bool m_isVisible;
};

} // namespace views
} // namespace media_player

#endif // LIBRARY_SCREEN_H