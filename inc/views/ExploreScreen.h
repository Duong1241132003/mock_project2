#ifndef EXPLORE_SCREEN_H
#define EXPLORE_SCREEN_H

// System includes
#include <memory>
#include <string>

// Project includes
#include "views/IView.h"

// Forward declarations
namespace media_player 
{
namespace controllers 
{
    class ExploreController;
}

namespace models 
{
    class ExploreModel;
}

namespace ui 
{
    class ImGuiManager;
}
}

namespace media_player 
{
namespace views 
{

/**
 * @brief View cho tính năng Explore — hiển thị thư viện theo cấu trúc folder.
 * 
 * Theo chuẩn MVC, View chỉ render UI và delegate tất cả action 
 * xuống ExploreController. Dữ liệu hiển thị được đọc từ ExploreModel.
 */
class ExploreScreen : public IView 
{
public:
    /**
     * @brief Constructs ExploreScreen với controller dependency.
     * @param exploreController Controller xử lý business logic
     * @param exploreModel Model chứa dữ liệu hiển thị
     */
    ExploreScreen(
        std::shared_ptr<controllers::ExploreController> exploreController,
        std::shared_ptr<models::ExploreModel> exploreModel
    );
    
    ~ExploreScreen() override;
    
    // ==================== IView Interface ====================
    
    void show() override;
    void hide() override;
    void update() override;
    bool isVisible() const override;
    void render(ui::ImGuiManager& painter) override;
    bool handleInput(const SDL_Event& event) override;

private:
    // ==================== Render Helpers ====================
    
    /**
     * @brief Render breadcrumb navigation bar.
     */
    void renderBreadcrumb(ui::ImGuiManager& painter, int x, int y, int w);
    
    /**
     * @brief Render context menu cho file.
     */
    void renderContextMenu(ui::ImGuiManager& painter);
    
    // ==================== Dependencies ====================
    
    std::shared_ptr<controllers::ExploreController> m_exploreController; ///< Controller
    std::shared_ptr<models::ExploreModel> m_exploreModel;               ///< Model (read-only)
    
    // ==================== View State (UI-only, không phải data) ====================
    
    bool m_isVisible;          ///< Trạng thái hiển thị
    int m_scrollOffset;        ///< Scroll offset cho danh sách
    std::string m_searchQuery; ///< Cache search query từ global state
    
    // Context menu state (UI state, nằm ở View)
    bool m_showContextMenu;    ///< Đang hiển thị context menu
    int m_contextMenuX;        ///< Vị trí X context menu
    int m_contextMenuY;        ///< Vị trí Y context menu
    int m_contextMenuIndex;    ///< Index file đang chọn trong context menu
};

} // namespace views
} // namespace media_player

#endif // EXPLORE_SCREEN_H
