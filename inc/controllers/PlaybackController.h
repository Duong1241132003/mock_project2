#ifndef PLAYBACK_CONTROLLER_H
#define PLAYBACK_CONTROLLER_H

// System includes
#include <memory>
#include <string>
#include <mutex>
#include <optional>

// Project includes
#include "services/IPlaybackEngine.h"
#include "services/AudioPlaybackEngine.h"
#include "models/QueueModel.h"
#include "models/PlaybackStateModel.h"
#include "models/MediaFileModel.h"
#include "repositories/HistoryRepository.h"
#include "utils/Observer.h"

namespace media_player 
{
namespace controllers 
{

class PlaybackController 
{
public:
    PlaybackController(
        std::shared_ptr<models::QueueModel> queueModel,
        std::shared_ptr<models::PlaybackStateModel> playbackStateModel,
        std::shared_ptr<repositories::HistoryRepository> historyRepo
    );
    
    ~PlaybackController();
    
    // Playback control
    bool play();
    bool pause();
    bool togglePlayPause();  // Toggle between play and pause
    bool stop();
    bool playNext();
    bool playPrevious();
    bool playItemAt(size_t index);
    /** Play media directly without modifying queue. When finished, stops (does not resume queue). */
    bool playMediaWithoutQueue(const models::MediaFileModel& media);
    bool seek(int positionSeconds);
    void onFinished();
    
    // Volume control
    void setVolume(int volume);
    int getVolume() const;
    
    // State queries
    bool isPlaying() const;
    bool isPaused() const;
    bool isStopped() const;
    
    // Current media info
    std::string getCurrentFilePath() const;
    models::MediaType getCurrentMediaType() const;
    
    void setAudioEngine(std::unique_ptr<services::IPlaybackEngine> engine);
    
private:
    // Engine management
    bool selectAndLoadEngine(const models::MediaFileModel& media);
    void cleanupCurrentEngine();
    
    // Callbacks from engines
    void onStateChanged(services::PlaybackState state);
    void onPositionChanged(int currentSeconds, int totalSeconds);
    void onError(const std::string& error);
    // Models
    std::shared_ptr<models::QueueModel> m_queueModel;
    std::shared_ptr<models::PlaybackStateModel> m_playbackStateModel;
    std::shared_ptr<repositories::HistoryRepository> m_historyRepo;
    
    // Engines
    std::unique_ptr<services::IPlaybackEngine> m_audioEngine;
    
    // Current active engine
    services::IPlaybackEngine* m_currentEngine;
    models::MediaType m_currentMediaType;
    
    /** When true, play() will not add current track to history (used for play previous). */
    bool m_skipHistoryOnNextPlay = false;
    /** When true, playing a song from history - onFinished will resume queue instead of playNext. */
    bool m_playingFromHistory = false;
    /** When true, playing one-off (e.g. from Library click) - onFinished will stop. */
    bool m_playingOneOffWithoutQueue = false;
    /** Stored when playing one-off, used for Loop One reload when seek(0) fails. */
    std::optional<models::MediaFileModel> m_oneOffMedia;
    
    // Thread safety
    mutable std::recursive_mutex m_mutex;
};

} // namespace controllers
} // namespace media_player

#endif // PLAYBACK_CONTROLLER_H
