// Project includes
#include "views/VideoRenderPanel.h"
#include "utils/Logger.h"

namespace media_player 
{
namespace views 
{

VideoRenderPanel::VideoRenderPanel(std::shared_ptr<controllers::VideoController> videoController)
    : m_videoController(videoController)
{
    LOG_INFO("VideoRenderPanel created");
}

VideoRenderPanel::~VideoRenderPanel() 
{
    LOG_INFO("VideoRenderPanel destroyed");
}

void VideoRenderPanel::render() 
{
    m_videoController->renderCurrentFrame();
}

void VideoRenderPanel::clear() 
{
    SDL_Renderer* renderer = m_videoController->getRenderer();
    
    if (renderer) 
    {
        SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);
        SDL_RenderClear(renderer);
        SDL_RenderPresent(renderer);
    }
}

} // namespace views
} // namespace media_player