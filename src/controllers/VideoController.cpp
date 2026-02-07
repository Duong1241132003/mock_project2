// Project includes
#include "controllers/VideoController.h"
#include "utils/Logger.h"
#include "config/AppConfig.h"

namespace media_player 
{
namespace controllers 
{

VideoController::VideoController(
    std::shared_ptr<PlaybackController> playbackController,
    std::shared_ptr<models::PlaybackStateModel> playbackStateModel
)
    : m_playbackController(playbackController)
    , m_playbackStateModel(playbackStateModel)
    , m_window(nullptr)
    , m_renderer(nullptr)
    , m_windowWidth(config::AppConfig::DEFAULT_VIDEO_WIDTH)
    , m_windowHeight(config::AppConfig::DEFAULT_VIDEO_HEIGHT)
{
    LOG_INFO("VideoController initialized");
}

VideoController::~VideoController() 
{
    destroyVideoWindow();
    LOG_INFO("VideoController destroyed");
}

bool VideoController::createVideoWindow(int width, int height) 
{
    if (m_window) 
    {
        LOG_WARNING("Video window already exists");
        return true;
    }
    
    m_windowWidth = width;
    m_windowHeight = height;
    
    m_window = SDL_CreateWindow(
        "Media Player - Video",
        SDL_WINDOWPOS_CENTERED,
        SDL_WINDOWPOS_CENTERED,
        width,
        height,
        SDL_WINDOW_SHOWN | SDL_WINDOW_RESIZABLE
    );
    
    if (!m_window) 
    {
        LOG_ERROR("Failed to create video window: " + std::string(SDL_GetError()));
        return false;
    }
    
    m_renderer = SDL_CreateRenderer(
        m_window,
        -1,
        SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC
    );
    
    if (!m_renderer) 
    {
        LOG_ERROR("Failed to create renderer: " + std::string(SDL_GetError()));
        SDL_DestroyWindow(m_window);
        m_window = nullptr;
        return false;
    }
    
    LOG_INFO("Video window created: " + std::to_string(width) + "x" + std::to_string(height));
    
    return true;
}

void VideoController::destroyVideoWindow() 
{
    if (m_renderer) 
    {
        SDL_DestroyRenderer(m_renderer);
        m_renderer = nullptr;
    }
    
    if (m_window) 
    {
        SDL_DestroyWindow(m_window);
        m_window = nullptr;
    }
    
    LOG_INFO("Video window destroyed");
}

bool VideoController::isWindowCreated() const 
{
    return m_window != nullptr;
}

void VideoController::toggleFullscreen() 
{
    bool currentFullscreen = m_playbackStateModel->isFullscreen();
    setFullscreen(!currentFullscreen);
}

void VideoController::setFullscreen(bool fullscreen) 
{
    if (!m_window) 
    {
        LOG_WARNING("No window to set fullscreen");
        return;
    }
    
    Uint32 flags = fullscreen ? SDL_WINDOW_FULLSCREEN_DESKTOP : 0;
    
    if (SDL_SetWindowFullscreen(m_window, flags) != 0) 
    {
        LOG_ERROR("Failed to set fullscreen: " + std::string(SDL_GetError()));
        return;
    }
    
    m_playbackStateModel->setFullscreen(fullscreen);
    
    LOG_INFO("Fullscreen " + std::string(fullscreen ? "enabled" : "disabled"));
}

bool VideoController::isFullscreen() const 
{
    return m_playbackStateModel->isFullscreen();
}

void VideoController::setAspectRatio(double ratio) 
{
    m_playbackStateModel->setAspectRatio(ratio);
    LOG_INFO("Aspect ratio set to: " + std::to_string(ratio));
}

double VideoController::getAspectRatio() const 
{
    return m_playbackStateModel->getAspectRatio();
}

void VideoController::fitToWindow() 
{
    if (!m_window) 
    {
        return;
    }
    
    int windowWidth, windowHeight;
    SDL_GetWindowSize(m_window, &windowWidth, &windowHeight);
    
    m_windowWidth = windowWidth;
    m_windowHeight = windowHeight;
    
    LOG_INFO("Video fitted to window");
}

void VideoController::originalSize() 
{
    int videoWidth, videoHeight;
    m_playbackController->getVideoResolution(videoWidth, videoHeight);
    
    if (videoWidth > 0 && videoHeight > 0 && m_window) 
    {
        SDL_SetWindowSize(m_window, videoWidth, videoHeight);
        m_windowWidth = videoWidth;
        m_windowHeight = videoHeight;
        
        LOG_INFO("Video set to original size");
    }
}

void VideoController::renderCurrentFrame() 
{
    if (!m_renderer) 
    {
        return;
    }
    
    SDL_Texture* texture = m_playbackController->getCurrentVideoTexture();
    
    if (!texture) 
    {
        // Clear to black
        SDL_SetRenderDrawColor(m_renderer, 0, 0, 0, 255);
        SDL_RenderClear(m_renderer);
        SDL_RenderPresent(m_renderer);
        return;
    }
    
    SDL_Rect renderRect;
    calculateRenderRect(renderRect);
    
    SDL_RenderClear(m_renderer);
    SDL_RenderCopy(m_renderer, texture, nullptr, &renderRect);
    SDL_RenderPresent(m_renderer);
}

void VideoController::calculateRenderRect(SDL_Rect& rect) 
{
    int videoWidth, videoHeight;
    m_playbackController->getVideoResolution(videoWidth, videoHeight);
    
    if (videoWidth == 0 || videoHeight == 0) 
    {
        rect.x = 0;
        rect.y = 0;
        rect.w = m_windowWidth;
        rect.h = m_windowHeight;
        return;
    }
    
    double videoAspect = static_cast<double>(videoWidth) / videoHeight;
    double windowAspect = static_cast<double>(m_windowWidth) / m_windowHeight;
    
    if (videoAspect > windowAspect) 
    {
        // Video is wider - fit to width
        rect.w = m_windowWidth;
        rect.h = static_cast<int>(m_windowWidth / videoAspect);
        rect.x = 0;
        rect.y = (m_windowHeight - rect.h) / 2;
    }
    else 
    {
        // Video is taller - fit to height
        rect.h = m_windowHeight;
        rect.w = static_cast<int>(m_windowHeight * videoAspect);
        rect.y = 0;
        rect.x = (m_windowWidth - rect.w) / 2;
    }
}

} // namespace controllers
} // namespace media_player