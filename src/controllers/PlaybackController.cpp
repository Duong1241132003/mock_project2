// Project includes
#include "controllers/PlaybackController.h"
#include "utils/Logger.h"

namespace media_player 
{
namespace controllers 
{

PlaybackController::PlaybackController(
    std::shared_ptr<models::QueueModel> queueModel,
    std::shared_ptr<models::PlaybackStateModel> playbackStateModel,
    std::shared_ptr<repositories::HistoryRepository> historyRepo
)
    : m_queueModel(queueModel)
    , m_playbackStateModel(playbackStateModel)
    , m_historyRepo(historyRepo)
    , m_currentEngine(nullptr)
    , m_currentMediaType(models::MediaType::UNKNOWN)
{
    // Create both engines
    m_audioEngine = std::make_unique<services::AudioPlaybackEngine>();
    m_videoEngine = std::make_unique<services::VideoPlaybackEngine>();
    
    // Setup callbacks for both engines
    m_audioEngine->setStateChangeCallback(
        [this](services::PlaybackState state) 
        {
            onStateChanged(state);
        }
    );
    
    m_audioEngine->setPositionCallback(
        [this](int current, int total) 
        {
            onPositionChanged(current, total);
        }
    );
    
    m_audioEngine->setErrorCallback(
        [this](const std::string& error) 
        {
            onError(error);
        }
    );
    
    m_audioEngine->setFinishedCallback(
        [this]() 
        {
            onFinished();
        }
    );
    
    m_videoEngine->setStateChangeCallback(
        [this](services::PlaybackState state) 
        {
            onStateChanged(state);
        }
    );
    
    m_videoEngine->setPositionCallback(
        [this](int current, int total) 
        {
            onPositionChanged(current, total);
        }
    );
    
    m_videoEngine->setErrorCallback(
        [this](const std::string& error) 
        {
            onError(error);
        }
    );
    
    m_videoEngine->setFinishedCallback(
        [this]() 
        {
            onFinished();
        }
    );
    
    LOG_INFO("PlaybackController initialized");
}

PlaybackController::~PlaybackController() 
{
    stop();
    LOG_INFO("PlaybackController destroyed");
}

bool PlaybackController::play() 
{
    std::lock_guard<std::recursive_mutex> lock(m_mutex);
    
    m_oneOffMedia.reset();
    m_playingOneOffWithoutQueue = false;  // play() is for queue; clear one-off state
    auto currentItem = m_queueModel->getCurrentItem();
    
    if (!currentItem) 
    {
        LOG_WARNING("No item in queue to play");
        return false;
    }
    
    // Skip items whose file no longer exists (deleted on disk)
    while (currentItem && !std::filesystem::exists(currentItem->getFilePath()))
    {
        LOG_WARNING("File no longer exists, skipping: " + currentItem->getFilePath());
        m_queueModel->removeByPath(currentItem->getFilePath());
        cleanupCurrentEngine();
        currentItem = m_queueModel->getCurrentItem();
    }
    if (!currentItem)
    {
        LOG_WARNING("No valid item in queue to play");
        stop();
        return false;
    }
    
    // Always sync Metadata, even if engine is already loaded for this file
    const auto& media = *currentItem;
    m_playbackStateModel->setCurrentTitle(media.getTitle().empty() ? media.getFileName() : media.getTitle());
    m_playbackStateModel->setCurrentArtist(media.getArtist().empty() ? "Unknown Artist" : media.getArtist());
    m_playbackStateModel->setCurrentMediaType(media.getType());

    // Check if we need to load a new file
    if (m_playbackStateModel->getCurrentFilePath() != currentItem->getFilePath()) 
    {
        if (!selectAndLoadEngine(*currentItem)) 
        {
            LOG_ERROR("Failed to load media file (may have been deleted): " + currentItem->getFilePath());
            m_queueModel->removeByPath(currentItem->getFilePath());
            cleanupCurrentEngine();
            return play();
        }
        
        m_playbackStateModel->setCurrentFilePath(currentItem->getFilePath());
        // MediaType set above
    }
    
    if (!m_currentEngine) 
    {
        LOG_ERROR("No engine available");
        return false;
    }
    
    bool success = m_currentEngine->play();
    
    if (success) 
    {
        // Add to history (skip when coming from play previous)
        if (m_skipHistoryOnNextPlay)
        {
            m_skipHistoryOnNextPlay = false;
        }
        else if (m_historyRepo)
        {
            m_historyRepo->removeAllEntriesByFilePath(currentItem->getFilePath());
            m_historyRepo->addEntry(*currentItem);
        }
        
        LOG_INFO("Playback started: " + currentItem->getFileName());
    }
    
    return success;
}

bool PlaybackController::pause() 
{
    std::lock_guard<std::recursive_mutex> lock(m_mutex);
    
    if (!m_currentEngine) 
    {
        return false;
    }
    
    return m_currentEngine->pause();
}

bool PlaybackController::togglePlayPause() 
{
    std::lock_guard<std::recursive_mutex> lock(m_mutex);
    
    if (isPlaying()) 
    {
        return pause();
    }
    else 
    {
        return play();
    }
}

bool PlaybackController::stop() 
{
    std::lock_guard<std::recursive_mutex> lock(m_mutex);
    
    if (!m_currentEngine) 
    {
        return false;
    }
    
    bool success = m_currentEngine->stop();
    
    if (success) 
    {
        m_playbackStateModel->setCurrentFilePath("");
        m_playbackStateModel->setCurrentMediaType(models::MediaType::UNKNOWN);
        m_playbackStateModel->setCurrentPosition(0);
        m_oneOffMedia.reset();
    }
    
    return success;
}

bool PlaybackController::playNext() 
{
    std::lock_guard<std::recursive_mutex> lock(m_mutex);
    bool moved = m_queueModel->moveToNext();
    if (moved) 
    {
        cleanupCurrentEngine();
        return play();
    }
    LOG_INFO("End of queue reached");
    stop();
    return false;
}

// First press: replay current (seek to 0), remove from history. When already at start (pos~0): play previous from history.
static const int REWIND_THRESHOLD_SEC = 2;

bool PlaybackController::playPrevious() 
{
    std::lock_guard<std::recursive_mutex> lock(m_mutex);
    
    auto currentItem = m_queueModel->getCurrentItem();
    int position = m_playbackStateModel->getCurrentPosition();
    bool hasCurrent = (currentItem && !m_playbackStateModel->getCurrentFilePath().empty());
    
    // First press (position > threshold): replay current (seek to 0) and remove it from history
    bool canReplay = hasCurrent && (isPlaying() || isPaused()) && (position > REWIND_THRESHOLD_SEC);
    if (canReplay)
    {
        if (m_historyRepo && currentItem)
            m_historyRepo->removeMostRecentEntryByFilePath(currentItem->getFilePath());
        return m_currentEngine ? m_currentEngine->seek(0) : false;
    }
    
    // Second press onwards: play the track that was played before current in history
    std::string currentPath = m_playbackStateModel->getCurrentFilePath();
    if (m_historyRepo)
    {
        std::optional<repositories::PlaybackHistoryEntry> prevPlayed = m_historyRepo->getPlayedBefore(currentPath);
        if (!prevPlayed && !currentPath.empty())
            prevPlayed = m_historyRepo->getLastPlayed(); // current was removed from history, so last in history is the previous
        if (prevPlayed && std::filesystem::exists(prevPlayed->media.getFilePath()))
        {
            m_historyRepo->removeMostRecentEntryByFilePath(prevPlayed->media.getFilePath());
            // Play directly without touching queue - when done, onFinished will resume queue
            cleanupCurrentEngine();
            if (!selectAndLoadEngine(prevPlayed->media))
                return false;
            m_playbackStateModel->setCurrentFilePath(prevPlayed->media.getFilePath());
            m_playbackStateModel->setCurrentTitle(prevPlayed->media.getTitle().empty() ? prevPlayed->media.getFileName() : prevPlayed->media.getTitle());
            m_playbackStateModel->setCurrentArtist(prevPlayed->media.getArtist().empty() ? "Unknown Artist" : prevPlayed->media.getArtist());
            m_playbackStateModel->setCurrentMediaType(prevPlayed->media.getType());
            m_playingFromHistory = true;
            return m_currentEngine ? m_currentEngine->play() : false;
        }
    }
    
    // Fallback: previous in queue (if no previous in history)
    if (!m_queueModel->hasPrevious())
    {
        LOG_INFO("No previous item in queue or history");
        return false;
    }
    
    m_queueModel->moveToPrevious();
    cleanupCurrentEngine();
    auto prevItem = m_queueModel->getCurrentItem();
    if (m_historyRepo && prevItem)
    {
        m_historyRepo->removeMostRecentEntryByFilePath(prevItem->getFilePath());
        m_skipHistoryOnNextPlay = true;
    }
    return play();
}

bool PlaybackController::playItemAt(size_t index) 
{
    std::lock_guard<std::recursive_mutex> lock(m_mutex);
    
    if (!m_queueModel->jumpTo(index)) 
    {
        LOG_ERROR("Failed to jump to index: " + std::to_string(index));
        return false;
    }
    
    cleanupCurrentEngine();
    return play();
}

bool PlaybackController::playMediaWithoutQueue(const models::MediaFileModel& media) 
{
    std::lock_guard<std::recursive_mutex> lock(m_mutex);
    
    if (!std::filesystem::exists(media.getFilePath()))
        return false;
    
    cleanupCurrentEngine();
    m_oneOffMedia = media;
    if (!selectAndLoadEngine(media))
        return false;
    
    m_playbackStateModel->setCurrentFilePath(media.getFilePath());
    m_playbackStateModel->setCurrentTitle(media.getTitle().empty() ? media.getFileName() : media.getTitle());
    m_playbackStateModel->setCurrentArtist(media.getArtist().empty() ? "Unknown Artist" : media.getArtist());
    m_playbackStateModel->setCurrentMediaType(media.getType());
    m_playingOneOffWithoutQueue = true;
    
    bool success = m_currentEngine ? m_currentEngine->play() : false;
    if (success && m_historyRepo)
    {
        m_historyRepo->removeAllEntriesByFilePath(media.getFilePath());
        m_historyRepo->addEntry(media);
    }
    return success;
}

bool PlaybackController::seek(int positionSeconds) 
{
    std::lock_guard<std::recursive_mutex> lock(m_mutex);
    
    if (!m_currentEngine) 
    {
        return false;
    }
    
    return m_currentEngine->seek(positionSeconds);
}

void PlaybackController::setVolume(int volume) 
{
    std::lock_guard<std::recursive_mutex> lock(m_mutex);
    
    if (m_currentEngine) 
    {
        m_currentEngine->setVolume(volume);
    }
    
    m_playbackStateModel->setVolume(volume);
}

int PlaybackController::getVolume() const 
{
    std::lock_guard<std::recursive_mutex> lock(m_mutex);
    return m_playbackStateModel->getVolume();
}

bool PlaybackController::isPlaying() const 
{
    std::lock_guard<std::recursive_mutex> lock(m_mutex);
    return m_playbackStateModel->isPlaying();
}

bool PlaybackController::isPaused() const 
{
    std::lock_guard<std::recursive_mutex> lock(m_mutex);
    return m_playbackStateModel->isPaused();
}

bool PlaybackController::isStopped() const 
{
    std::lock_guard<std::recursive_mutex> lock(m_mutex);
    return m_playbackStateModel->isStopped();
}

std::string PlaybackController::getCurrentFilePath() const 
{
    std::lock_guard<std::recursive_mutex> lock(m_mutex);
    return m_playbackStateModel->getCurrentFilePath();
}

models::MediaType PlaybackController::getCurrentMediaType() const 
{
    std::lock_guard<std::recursive_mutex> lock(m_mutex);
    return m_playbackStateModel->getCurrentMediaType();
}

void PlaybackController::setFullscreen(bool fullscreen) 
{
    std::lock_guard<std::recursive_mutex> lock(m_mutex);
    m_playbackStateModel->setFullscreen(fullscreen);
    
    // TODO: Implement SDL window fullscreen toggle
    LOG_INFO("Fullscreen " + std::string(fullscreen ? "enabled" : "disabled"));
}

bool PlaybackController::isFullscreen() const 
{
    std::lock_guard<std::recursive_mutex> lock(m_mutex);
    return m_playbackStateModel->isFullscreen();
}

SDL_Texture* PlaybackController::getCurrentVideoTexture() 
{
    std::lock_guard<std::recursive_mutex> lock(m_mutex);
    if (m_currentMediaType == models::MediaType::VIDEO && m_videoEngine) 
    {
        return m_videoEngine->getCurrentVideoTexture();
    }
    
    return nullptr;
}

void PlaybackController::getVideoResolution(int& width, int& height) 
{
    std::lock_guard<std::recursive_mutex> lock(m_mutex);
    if (m_currentMediaType == models::MediaType::VIDEO && m_videoEngine) 
    {
        m_videoEngine->getVideoResolution(width, height);
    }
    else 
    {
        width = 0;
        height = 0;
    }
}

void PlaybackController::presentVideoFrame()
{
    std::lock_guard<std::recursive_mutex> lock(m_mutex);
    if (m_currentMediaType == models::MediaType::VIDEO && m_videoEngine)
    {
        m_videoEngine->presentVideoFrame();
    }
}

void PlaybackController::updateTextureFromMainThread()
{
    std::lock_guard<std::recursive_mutex> lock(m_mutex);
    if (m_currentMediaType == models::MediaType::VIDEO && m_videoEngine)
    {
        m_videoEngine->updateTextureFromMainThread();
    }
}

void PlaybackController::setExternalRenderer(SDL_Renderer* renderer)
{
    std::lock_guard<std::recursive_mutex> lock(m_mutex);
    if (m_videoEngine)
    {
        m_videoEngine->setExternalRenderer(renderer);
    }
}

bool PlaybackController::isPlayingVideo() const
{
    std::lock_guard<std::recursive_mutex> lock(m_mutex);
    return m_currentMediaType == models::MediaType::VIDEO && 
           m_currentEngine != nullptr &&
           m_playbackStateModel->getState() != models::PlaybackState::STOPPED;
}

bool PlaybackController::selectAndLoadEngine(const models::MediaFileModel& media) 
{
    models::MediaType newType = media.getType();
    
    // Explicitly release resources if switching engine types
    if (m_currentEngine && m_currentMediaType != newType)
    {
        m_currentEngine->stop();
        m_currentEngine->releaseResources();
    }
    else
    {
        cleanupCurrentEngine();
    }
    
    m_currentMediaType = newType;
    
    if (newType == models::MediaType::AUDIO) 
    {
        // Use AudioEngine (SDL_mixer) only for simple formats like MP3/WAV if desired.
        // But for reliability with .m4a, .ogg, .flac on some systems, FFmpeg (VideoEngine) is better.
        // Check extension
        std::string ext = "";
        std::string path = media.getFilePath();
        size_t dotPos = path.rfind('.');
        if (dotPos != std::string::npos) {
            ext = path.substr(dotPos);
            // Lowercase
            std::transform(ext.begin(), ext.end(), ext.begin(), ::tolower);
        }
        
        if (ext == ".m4a" || ext == ".ogg" || ext == ".flac") 
        {
            LOG_INFO("Using Universal/Video Engine for complex audio format: " + ext);
            m_currentEngine = m_videoEngine.get();
            // Treat as audio but processed by video engine
            m_currentMediaType = models::MediaType::VIDEO; // Hack to keep engine happy or valid logic
            // Actually VideoEngine supports audio-only files too if we set type to VIDEO or if engine supports AUDIO?
            // VideoEngine::supportsMediaType checks for VIDEO. 
            // We can just CAST it to use the pointer but PlaybackController tracks m_currentMediaType.
            // Let's set m_currentMediaType = models::MediaType::VIDEO so getter uses video engine.
            // Is that safe? yes, PlaybackController::getCurrentVideoTexture checks this.
        }
        else 
        {
            LOG_INFO("Selecting audio engine for: " + media.getFileName());
            m_currentEngine = m_audioEngine.get();
        }
    }
    else if (newType == models::MediaType::VIDEO) 
    {
        LOG_INFO("Selecting video engine for: " + media.getFileName());
        m_currentEngine = m_videoEngine.get();
        m_currentMediaType = models::MediaType::VIDEO;
    }
    else 
    {
        LOG_ERROR("Unsupported media type");
        return false;
    }
    
    if (!m_currentEngine->loadFile(media.getFilePath())) 
    {
        LOG_ERROR("Failed to load file: " + media.getFilePath());
        m_currentEngine = nullptr;
        m_currentMediaType = models::MediaType::UNKNOWN;
        return false;
    }
    
    // Sync volume
    m_currentEngine->setVolume(m_playbackStateModel->getVolume());
    
    // Metadata is now handled in play() to ensure it's always up to date
    
    return true;
}

void PlaybackController::cleanupCurrentEngine() 
{
    if (m_currentEngine) 
    {
        m_currentEngine->stop();
    }
    
    m_currentEngine = nullptr;
    m_currentMediaType = models::MediaType::UNKNOWN;
}

void PlaybackController::onStateChanged(services::PlaybackState state) 
{
    std::lock_guard<std::recursive_mutex> lock(m_mutex);
    models::PlaybackState modelState;
    
    switch (state) 
    {
        case services::PlaybackState::PLAYING:
            modelState = models::PlaybackState::PLAYING;
            break;
        case services::PlaybackState::PAUSED:
            modelState = models::PlaybackState::PAUSED;
            break;
        case services::PlaybackState::STOPPED:
            modelState = models::PlaybackState::STOPPED;
            break;
    }
    
    m_playbackStateModel->setState(modelState);
    
    LOG_INFO("Playback state changed");
}

void PlaybackController::onPositionChanged(int currentSeconds, int totalSeconds) 
{
    std::lock_guard<std::recursive_mutex> lock(m_mutex);
    m_playbackStateModel->setCurrentPosition(currentSeconds);
    m_playbackStateModel->setTotalDuration(totalSeconds);
}

void PlaybackController::onError(const std::string& error) 
{
    std::lock_guard<std::recursive_mutex> lock(m_mutex);
    LOG_ERROR("Playback error: " + error);
    
    bool wasFromHistory = m_playingFromHistory;
    bool wasOneOff = m_playingOneOffWithoutQueue;
    m_playingFromHistory = false;
    m_playingOneOffWithoutQueue = false;
    
    std::string currentPath = m_playbackStateModel->getCurrentFilePath();
    if (!wasFromHistory && !wasOneOff && !currentPath.empty())
        m_queueModel->removeByPath(currentPath);
    cleanupCurrentEngine();
    
    if (!wasOneOff && !m_queueModel->isEmpty())
        play();
    else
        stop();
}

void PlaybackController::onFinished() 
{
    std::lock_guard<std::recursive_mutex> lock(m_mutex);
    LOG_INFO("Playback finished");
    bool isLoopOne = m_queueModel->isLoopOneEnabled();
    if (m_playingOneOffWithoutQueue)
    {
        m_playingOneOffWithoutQueue = false;
        // Loop One only (Loop All N/A for one-off - no queue)
        if (isLoopOne && m_currentEngine)
        {
            bool seekOk = m_currentEngine->seek(0);
            if (seekOk)
            {
                m_playingOneOffWithoutQueue = true;
                m_currentEngine->play();
                return;
            }
            // seek failed (common when track just finished) - reload and play
            if (m_oneOffMedia && std::filesystem::exists(m_oneOffMedia->getFilePath()))
            {
                const auto& media = *m_oneOffMedia;
                cleanupCurrentEngine();
                if (selectAndLoadEngine(media))
                {
                    m_playbackStateModel->setCurrentFilePath(media.getFilePath());
                    m_playbackStateModel->setCurrentTitle(media.getTitle().empty() ? media.getFileName() : media.getTitle());
                    m_playbackStateModel->setCurrentArtist(media.getArtist().empty() ? "Unknown Artist" : media.getArtist());
                    m_playbackStateModel->setCurrentMediaType(media.getType());
                    m_playingOneOffWithoutQueue = true;
                    if (m_currentEngine && m_currentEngine->play())
                        return;
                }
            }
        }
        cleanupCurrentEngine();
        stop();
    }
    else if (m_playingFromHistory)
    {
        m_playingFromHistory = false;
        cleanupCurrentEngine();
        play(); // Resume queue (don't advance - play current item)
    }
    else
    {
        // Playing from queue
        if (isLoopOne)
        {
            if (m_currentEngine)
            {
                bool seekOk = m_currentEngine->seek(0);
                if (seekOk)
                {
                    m_currentEngine->play();
                    return;
                }
            }
            // seek failed - reload current track and play
            auto currentItem = m_queueModel->getCurrentItem();
            if (currentItem && std::filesystem::exists(currentItem->getFilePath()))
            {
                cleanupCurrentEngine();
                m_playbackStateModel->setCurrentFilePath("");
                if (play())
                    return;
            }
        }
        playNext(); // Loop All: moveToNext wraps; None: stop at end
    }
}

} // namespace controllers
} // namespace media_player