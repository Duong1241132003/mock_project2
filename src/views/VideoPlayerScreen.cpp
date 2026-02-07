// Project includes
#include "views/VideoPlayerScreen.h"
#include "utils/Logger.h"

// System includes
#include <chrono>

namespace media_player 
{
namespace views 
{

VideoPlayerScreen::VideoPlayerScreen(
    std::shared_ptr<controllers::VideoController> videoController,
    std::shared_ptr<controllers::PlaybackController> playbackController,
    std::shared_ptr<models::PlaybackStateModel> playbackStateModel
)
    : m_videoController(videoController)
    , m_playbackController(playbackController)
    , m_playbackStateModel(playbackStateModel)
    , m_isVisible(false)
    , m_controlsVisible(true)
{
    m_renderPanel = std::make_unique<VideoRenderPanel>(videoController);
    
    LOG_INFO("VideoPlayerScreen created");
}

VideoPlayerScreen::~VideoPlayerScreen() 
{
    destroyWindow();
    LOG_INFO("VideoPlayerScreen destroyed");
}

void VideoPlayerScreen::show() 
{
    if (!createWindow()) 
    {
        LOG_ERROR("Failed to create video window");
        return;
    }
    
    m_isVisible = true;
    LOG_INFO("VideoPlayerScreen shown");
}

void VideoPlayerScreen::hide() 
{
    m_isVisible = false;
    destroyWindow();
    LOG_INFO("VideoPlayerScreen hidden");
}

void VideoPlayerScreen::update() 
{
    if (!m_isVisible) 
    {
        return;
    }
    
    updateWindowTitle();
    renderFrame();
    
    // Auto-hide controls after 3 seconds of inactivity
    auto now = std::chrono::steady_clock::now();
    auto elapsed = std::chrono::duration_cast<std::chrono::seconds>(
        now - m_lastMouseMovement
    ).count();
    
    if (m_controlsVisible && elapsed > 3) 
    {
        hideControls();
    }
}

bool VideoPlayerScreen::isVisible() const 
{
    return m_isVisible;
}

bool VideoPlayerScreen::createWindow() 
{
    int width = 1280;
    int height = 720;
    
    // Get video resolution if available
    m_playbackController->getVideoResolution(width, height);
    
    if (width == 0 || height == 0) 
    {
        width = 1280;
        height = 720;
    }
    
    return m_videoController->createVideoWindow(width, height);
}

void VideoPlayerScreen::destroyWindow() 
{
    m_videoController->destroyVideoWindow();
}

void VideoPlayerScreen::handleKeyPress(SDL_Keycode key) 
{
    switch (key) 
    {
        case SDLK_SPACE:
            if (m_playbackStateModel->isPlaying()) 
            {
                m_playbackController->pause();
            }
            else 
            {
                m_playbackController->play();
            }
            break;
            
        case SDLK_f:
        case SDLK_F11:
            m_videoController->toggleFullscreen();
            break;
            
        case SDLK_ESCAPE:
            if (m_videoController->isFullscreen()) 
            {
                m_videoController->setFullscreen(false);
            }
            else 
            {
                hide();
            }
            break;
            
        case SDLK_LEFT:
            {
                int currentPos = m_playbackStateModel->getCurrentPosition();
                m_playbackController->seek(currentPos - 10);
            }
            break;
            
        case SDLK_RIGHT:
            {
                int currentPos = m_playbackStateModel->getCurrentPosition();
                m_playbackController->seek(currentPos + 10);
            }
            break;
            
        case SDLK_UP:
            {
                int volume = m_playbackController->getVolume();
                m_playbackController->setVolume(volume + 5);
            }
            break;
            
        case SDLK_DOWN:
            {
                int volume = m_playbackController->getVolume();
                m_playbackController->setVolume(volume - 5);
            }
            break;
            
        default:
            break;
    }
    
    showControls();
}

void VideoPlayerScreen::handleMouseClick(int x, int y) 
{
    toggleControls();
    m_lastMouseMovement = std::chrono::steady_clock::now();
}

void VideoPlayerScreen::handleWindowResize(int width, int height) 
{
    m_videoController->fitToWindow();
}

void VideoPlayerScreen::showControls() 
{
    m_controlsVisible = true;
    m_lastMouseMovement = std::chrono::steady_clock::now();
}

void VideoPlayerScreen::hideControls() 
{
    m_controlsVisible = false;
}

void VideoPlayerScreen::toggleControls() 
{
    if (m_controlsVisible) 
    {
        hideControls();
    }
    else 
    {
        showControls();
    }
}

void VideoPlayerScreen::renderFrame() 
{
    if (m_renderPanel) 
    {
        m_renderPanel->render();
    }
    
    if (m_controlsVisible) 
    {
        renderControlsOverlay();
    }
}

void VideoPlayerScreen::renderControlsOverlay() 
{
    // TODO: Render semi-transparent overlay with playback controls
    // This would typically use SDL_Renderer or GUI framework
}

void VideoPlayerScreen::updateWindowTitle() 
{
    SDL_Window* window = m_videoController->getWindow();
    
    if (!window) 
    {
        return;
    }
    
    std::string title = "Media Player - Video";
    
    std::string currentPath = m_playbackStateModel->getCurrentFilePath();
    if (!currentPath.empty()) 
    {
        size_t lastSlash = currentPath.find_last_of("/\\");
        std::string fileName = (lastSlash != std::string::npos) 
            ? currentPath.substr(lastSlash + 1) 
            : currentPath;
        
        title = fileName + " - Media Player";
    }
    
    SDL_SetWindowTitle(window, title.c_str());
}

} // namespace views
} // namespace media_player