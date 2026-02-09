#ifndef LIBRARY_SCREEN_H
#define LIBRARY_SCREEN_H

// System includes
#include <memory>
#include <vector>

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

class LibraryScreen : public IView 
{
public:
    LibraryScreen(
        std::shared_ptr<controllers::LibraryController> libraryController,
        std::shared_ptr<controllers::QueueController> queueController,
        std::shared_ptr<controllers::PlaybackController> playbackController,
        std::shared_ptr<controllers::PlaylistController> playlistController
    );
    
    ~LibraryScreen();
    
    // IView interface
    void show() override;
    void hide() override;
    void update() override;
    bool isVisible() const override;
    
    // New ImGui View Interface
    void render(ui::ImGuiManager& painter) override;
    bool handleInput(const SDL_Event& event) override;
    
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
    
    // Search & Filter State
    std::string m_searchQuery;
    int m_sortField; // 0=Title, 1=Artist, 2=Album, 3=Duration
    bool m_sortAscending;
    int m_searchFilter; // 0=All, 1=Title, 2=Artist, 3=Album
    bool m_isVisible;

    // Context Menu State
    bool m_showContextMenu;
    int m_contextMenuX;
    int m_contextMenuY;
    int m_contextMenuIndex; // Index in m_currentMediaList (or filtered list?)
                            // Better to store index of filtered list so we can retrieve media
    
    // Dependencies
    std::shared_ptr<controllers::LibraryController> m_libraryController;
    std::shared_ptr<controllers::QueueController> m_queueController;
    std::shared_ptr<controllers::PlaybackController> m_playbackController;
    std::shared_ptr<controllers::PlaylistController> m_playlistController; // Added
    
    std::vector<models::MediaFileModel> m_currentMediaList;
    
    // UI State
    int m_currentPage;
    static constexpr int ITEMS_PER_PAGE = 25;
    int m_selectedIndex;
    int m_scrollOffset;
};

} // namespace views
} // namespace media_player

#endif // LIBRARY_SCREEN_H