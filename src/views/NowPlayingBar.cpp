// Project includes
#include "views/NowPlayingBar.h"

namespace media_player 
{
namespace views 
{

NowPlayingBar::NowPlayingBar(
    std::shared_ptr<models::PlaybackStateModel> playbackStateModel,
    std::shared_ptr<controllers::PlaybackController> playbackController
)
    : m_playbackStateModel(playbackStateModel)
    , m_playbackController(playbackController)
    , m_isVisible(false)
{
}

NowPlayingBar::~NowPlayingBar() 
{
}

void NowPlayingBar::show() 
{
    m_isVisible = true;
    update();
}

void NowPlayingBar::hide() 
{
    m_isVisible = false;
}

void NowPlayingBar::update() 
{
    if (!m_isVisible) 
    {
        return;
    }
    
    updateDisplayInfo();
    updateProgressBar();
    updatePlayPauseButton();
}

bool NowPlayingBar::isVisible() const 
{
    return m_isVisible;
}

void NowPlayingBar::onPlayPauseClicked() 
{
    m_playbackController->togglePlayPause();
}

void NowPlayingBar::onStopClicked() 
{
    m_playbackController->stop();
}

void NowPlayingBar::onNextClicked() 
{
    m_playbackController->playNext();
}

void NowPlayingBar::onPreviousClicked() 
{
    m_playbackController->playPrevious();
}

void NowPlayingBar::onVolumeChanged(int volume) 
{
    m_playbackController->setVolume(volume);
}

void NowPlayingBar::onSeekBarDragged(float percentage) 
{
    int totalDuration = m_playbackStateModel->getTotalDuration();
    int targetPosition = static_cast<int>(totalDuration * percentage / 100.0f);
    
    m_playbackController->seek(targetPosition);
}

void NowPlayingBar::updateDisplayInfo() 
{
    std::string currentTime = m_playbackStateModel->getFormattedPosition();
    std::string totalTime = m_playbackStateModel->getFormattedDuration();
    
    // TODO: Update UI labels
}

void NowPlayingBar::updateProgressBar() 
{
    float progress = m_playbackStateModel->getProgressPercentage();
    
    // TODO: Update UI progress bar
}

void NowPlayingBar::updatePlayPauseButton() 
{
    bool isPlaying = m_playbackStateModel->isPlaying();
    
    // TODO: Update UI button icon (Play/Pause)
}

} // namespace views
} // namespace media_player
