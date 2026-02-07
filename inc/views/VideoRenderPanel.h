#ifndef VIDEO_RENDER_PANEL_H
#define VIDEO_RENDER_PANEL_H

// System includes
#include <memory>

// SDL includes
#include <SDL2/SDL.h>

// Project includes
#include "controllers/VideoController.h"

namespace media_player 
{
namespace views 
{

class VideoRenderPanel 
{
public:
    explicit VideoRenderPanel(std::shared_ptr<controllers::VideoController> videoController);
    ~VideoRenderPanel();
    
    void render();
    void clear();
    
private:
    std::shared_ptr<controllers::VideoController> m_videoController;
};

} // namespace views
} // namespace media_player

#endif // VIDEO_RENDER_PANEL_H