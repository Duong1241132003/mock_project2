#ifndef VIDEO_CONTROLLER_H
#define VIDEO_CONTROLLER_H

// System includes
#include <memory>

// SDL includes
#include <SDL2/SDL.h>

// Project includes
#include "PlaybackController.h"
#include "models/PlaybackStateModel.h"

namespace media_player 
{
namespace controllers 
{

class VideoController 
{
public:
    VideoController(
        std::shared_ptr<PlaybackController> playbackController,
        std::shared_ptr<models::PlaybackStateModel> playbackStateModel
    );
    
    ~VideoController();
    
    // Window management
    bool createVideoWindow(int width, int height);
    void destroyVideoWindow();
    bool isWindowCreated() const;
    
    // Fullscreen control
    void toggleFullscreen();
    void setFullscreen(bool fullscreen);
    bool isFullscreen() const;
    
    // Aspect ratio
    void setAspectRatio(double ratio);
    double getAspectRatio() const;
    void fitToWindow();
    void originalSize();
    
    // Rendering
    void renderCurrentFrame();
    SDL_Renderer* getRenderer() const 
    { 
        return m_renderer; 
    }
    
    SDL_Window* getWindow() const 
    { 
        return m_window; 
    }
    
private:
    void calculateRenderRect(SDL_Rect& rect);
    
    std::shared_ptr<PlaybackController> m_playbackController;
    std::shared_ptr<models::PlaybackStateModel> m_playbackStateModel;
    
    SDL_Window* m_window;
    SDL_Renderer* m_renderer;
    
    int m_windowWidth;
    int m_windowHeight;
};

} // namespace controllers
} // namespace media_player

#endif // VIDEO_CONTROLLER_H