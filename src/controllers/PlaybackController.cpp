// Project includes
#include "controllers/PlaybackController.h"

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
    // Create only audio engine (video support removed)
    m_audioEngine = std::make_unique<services::AudioPlaybackEngine>();
    
    // Setup callbacks for audio engine only
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
    
}

PlaybackController::~PlaybackController() 
{
    stop();
}

bool PlaybackController::play() 
{
    std::lock_guard<std::recursive_mutex> lock(m_mutex);
    
    m_oneOffMedia.reset();
    m_playingOneOffWithoutQueue = false;  // play() is for queue; clear one-off state
    auto currentItem = m_queueModel->getCurrentItem();
    
    if (!currentItem) 
    {
        return false;
    }
    
    // Skip items whose file no longer exists (deleted on disk)
    while (currentItem && !std::filesystem::exists(currentItem->getFilePath()))
    {
        m_queueModel->removeByPath(currentItem->getFilePath());
        cleanupCurrentEngine();
        currentItem = m_queueModel->getCurrentItem();
    }
    if (!currentItem)
    {
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
            m_queueModel->removeByPath(currentItem->getFilePath());
            cleanupCurrentEngine();
            return play();
        }
        
        m_playbackStateModel->setCurrentFilePath(currentItem->getFilePath());
        // MediaType set above
    }
    
    if (!m_currentEngine) 
    {
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
    else if (isPaused() && m_currentEngine)
    {
        // Resume the current track without changing queue
        return m_currentEngine->play();
    }
    else 
    {
        // If stopped, check if we have one-off media to resume
        if (m_oneOffMedia && m_currentEngine)
        {
            // Check if engine has file loaded (can resume)
            if (!m_playbackStateModel->getCurrentFilePath().empty())
            {
                // Resume one-off media without clearing it
                return m_currentEngine->play();
            }
            else
            {
                // Engine lost file, reload one-off media
                if (selectAndLoadEngine(*m_oneOffMedia))
                {
                    m_playbackStateModel->setCurrentFilePath(m_oneOffMedia->getFilePath());
                    m_playbackStateModel->setCurrentTitle(m_oneOffMedia->getTitle().empty() ? m_oneOffMedia->getFileName() : m_oneOffMedia->getTitle());
                    m_playbackStateModel->setCurrentArtist(m_oneOffMedia->getArtist().empty() ? "Unknown Artist" : m_oneOffMedia->getArtist());
                    m_playbackStateModel->setCurrentMediaType(m_oneOffMedia->getType());
                    return m_currentEngine->play();
                }
            }
        }
        else
        {
            // If no one-off media, then play from queue
            return play();
        }
    }
    return false;
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

bool PlaybackController::selectAndLoadEngine(const models::MediaFileModel& media) 
{
    models::MediaType newType = media.getType();
    
    // Video support removed - only audio is supported
    // Also block unsupported files
    if (newType == models::MediaType::UNSUPPORTED)
    {
        return false;
    }

    if (newType != models::MediaType::AUDIO) 
    {
        return false;
    }
    
    // Cleanup current engine
    cleanupCurrentEngine();
    m_currentMediaType = models::MediaType::AUDIO;
    
    // Always use audio engine now (video engine removed)
    m_currentEngine = m_audioEngine.get();
    
    if (!m_currentEngine->loadFile(media.getFilePath())) 
    {
        m_currentEngine = nullptr;
        m_currentMediaType = models::MediaType::UNKNOWN;
        return false;
    }
    
    // Sync volume
    m_currentEngine->setVolume(m_playbackStateModel->getVolume());
    
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
    (void)error;
    
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

void PlaybackController::setAudioEngine(std::unique_ptr<services::IPlaybackEngine> engine)
{
    std::lock_guard<std::recursive_mutex> lock(m_mutex);
    m_audioEngine = std::move(engine);
    m_audioEngine->setStateChangeCallback([this](services::PlaybackState state) { onStateChanged(state); });
    m_audioEngine->setPositionCallback([this](int current, int total) { onPositionChanged(current, total); });
    m_audioEngine->setErrorCallback([this](const std::string& error) { onError(error); });
    m_audioEngine->setFinishedCallback([this]() { onFinished(); });
    m_currentEngine = m_audioEngine.get();
}
} // namespace controllers
} // namespace media_player
