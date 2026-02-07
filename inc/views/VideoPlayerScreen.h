#ifndef VIDEO_PLAYER_SCREEN_H
#define VIDEO_PLAYER_SCREEN_H

// System includes
#include <memory>

// SDL includes
#include <SDL2/SDL.h>

// Project includes
#include "IView.h"
#include "VideoRenderPanel.h"
#include "controllers/VideoController.h"
#include "controllers/PlaybackController.h"
#include "models/PlaybackStateModel.h"

namespace media_player 
{
namespace views 
{

class VideoPlayerScreen : public IView 
{
public:
    VideoPlayerScreen(
        std::shared_ptr<controllers::VideoController> videoController,
        std::shared_ptr<controllers::PlaybackController> playbackController,
        std::shared_ptr<models::PlaybackStateModel> playbackStateModel
    );
    
    ~VideoPlayerScreen();
    
    // IView interface
    void show() override;
    void hide() override;
    void update() override;
    bool isVisible() const override;
    
    // Window management
    bool createWindow();
    void destroyWindow();
    
    // Event handling
    void handleKeyPress(SDL_Keycode key);
    void handleMouseClick(int x, int y);
    void handleWindowResize(int width, int height);
    
    // Controls overlay
    void showControls();
    void hideControls();
    void toggleControls();
    
private:
    void renderFrame();
    void renderControlsOverlay();
    void updateWindowTitle();
    
    std::shared_ptr<controllers::VideoController> m_videoController;
    std::shared_ptr<controllers::PlaybackController> m_playbackController;
    std::shared_ptr<models::PlaybackStateModel> m_playbackStateModel;
    
    std::unique_ptr<VideoRenderPanel> m_renderPanel;
    
    bool m_isVisible;
    bool m_controlsVisible;
    std::chrono::steady_clock::time_point m_lastMouseMovement;
};

} // namespace views
} // namespace media_player

#endif // VIDEO_PLAYER_SCREEN_H