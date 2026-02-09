#ifndef I_VIEW_H
#define I_VIEW_H

// SDL Forward declaration
union SDL_Event;

namespace media_player 
{

// Forward declaration
namespace ui { class ImGuiManager; }

namespace views 
{

class IView 
{
public:
    virtual ~IView() = default;
    
    virtual void show() = 0;
    virtual void hide() = 0;
    virtual void update() = 0;
    virtual bool isVisible() const = 0;
    
    // New methods for Immediate Mode GUI
    virtual void render(ui::ImGuiManager& painter) {}; // Default empty for compatibility
    virtual bool handleInput(const SDL_Event& event) { return false; }; // Default empty
};

} // namespace views
} // namespace media_player

#endif // I_VIEW_H