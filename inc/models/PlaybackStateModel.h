#ifndef PLAYBACK_STATE_MODEL_H
#define PLAYBACK_STATE_MODEL_H

// System includes
#include <string>

// Project includes
// Project includes
#include "MediaFileModel.h"

// System includes
#include <functional>
#include <mutex>

namespace media_player 
{
namespace models 
{

enum class PlaybackState 
{
    STOPPED,
    PLAYING,
    PAUSED
};

class PlaybackStateModel 
{
public:
    PlaybackStateModel() = default;
    
    // State
    PlaybackState getState() const 
    { 
        return m_state; 
    }
    
    // Callbacks
    using StateChangeCallback = std::function<void(PlaybackState)>;
    using MetadataChangeCallback = std::function<void()>;
    using PositionChangeCallback = std::function<void(int)>;
    using DurationChangeCallback = std::function<void(int)>;
    
    void setStateChangeCallback(StateChangeCallback callback) { 
        std::lock_guard<std::mutex> lock(m_mutex);
        m_stateChangeCallback = callback; 
    }
    void setMetadataChangeCallback(MetadataChangeCallback callback) { 
        std::lock_guard<std::mutex> lock(m_mutex);
        m_metadataChangeCallback = callback; 
    }
    
    void setState(PlaybackState state) 
    { 
        std::lock_guard<std::mutex> lock(m_mutex);
        if (m_state != state) {
            m_state = state; 
            if (m_stateChangeCallback) m_stateChangeCallback(m_state);
        }
    }
    
    // Position
    int getCurrentPosition() const 
    { 
        std::lock_guard<std::mutex> lock(m_mutex);
        return m_currentPositionSeconds; 
    }
    
    void setCurrentPosition(int seconds) 
    { 
        std::lock_guard<std::mutex> lock(m_mutex);
        m_currentPositionSeconds = seconds; 
    }
    
    int getTotalDuration() const 
    { 
        std::lock_guard<std::mutex> lock(m_mutex);
        return m_totalDurationSeconds; 
    }
    
    void setTotalDuration(int seconds) 
    { 
        std::lock_guard<std::mutex> lock(m_mutex);
        m_totalDurationSeconds = seconds; 
    }
    
    // Volume
    int getVolume() const 
    { 
        std::lock_guard<std::mutex> lock(m_mutex);
        return m_volume; 
    }
    
    void setVolume(int volume) { 
        std::lock_guard<std::mutex> lock(m_mutex);
        m_volume = volume; 
    }
    
    // Current file
    std::string getCurrentFilePath() const 
    { 
        std::lock_guard<std::mutex> lock(m_mutex);
        return m_currentFilePath; 
    }
    
    void setCurrentFilePath(const std::string& path) 
    { 
        std::lock_guard<std::mutex> lock(m_mutex);
        m_currentFilePath = path; 
        if (m_metadataChangeCallback) m_metadataChangeCallback();
    }
    
    // Media type
    MediaType getCurrentMediaType() const 
    { 
        std::lock_guard<std::mutex> lock(m_mutex);
        return m_currentMediaType; 
    }
    
    void setCurrentMediaType(MediaType type) 
    { 
        std::lock_guard<std::mutex> lock(m_mutex);
        m_currentMediaType = type; 
    }
    
    // Video-specific state
    bool isFullscreen() const 
    { 
        std::lock_guard<std::mutex> lock(m_mutex);
        return m_isFullscreen; 
    }
    
    void setFullscreen(bool fullscreen) 
    { 
        std::lock_guard<std::mutex> lock(m_mutex);
        m_isFullscreen = fullscreen; 
    }
    
    double getAspectRatio() const 
    { 
        std::lock_guard<std::mutex> lock(m_mutex);
        return m_aspectRatio; 
    }
    
    void setAspectRatio(double ratio) 
    { 
        std::lock_guard<std::mutex> lock(m_mutex);
        m_aspectRatio = ratio; 
    }
    
    // Helpers
    bool isPlaying() const 
    { 
        std::lock_guard<std::mutex> lock(m_mutex);
        return m_state == PlaybackState::PLAYING; 
    }
    
    bool isPaused() const 
    { 
        std::lock_guard<std::mutex> lock(m_mutex);
        return m_state == PlaybackState::PAUSED; 
    }
    
    bool isStopped() const 
    { 
        std::lock_guard<std::mutex> lock(m_mutex);
        return m_state == PlaybackState::STOPPED; 
    }
    
    bool isPlayingVideo() const 
    { 
        std::lock_guard<std::mutex> lock(m_mutex);
        return m_currentMediaType == MediaType::VIDEO; 
    }
    
    bool isPlayingAudio() const 
    { 
        std::lock_guard<std::mutex> lock(m_mutex);
        return m_currentMediaType == MediaType::AUDIO; 
    }
    
    std::string getFormattedPosition() const;
    std::string getFormattedDuration() const;
    float getProgressPercentage() const;
    
    // Metadata
    std::string getCurrentTitle() const { 
        std::lock_guard<std::mutex> lock(m_mutex);
        return m_currentTitle; 
    }
    void setCurrentTitle(const std::string& title) { 
        {
            std::lock_guard<std::mutex> lock(m_mutex);
            m_currentTitle = title; 
        }
        if (m_metadataChangeCallback) m_metadataChangeCallback();
    }
    
    std::string getCurrentArtist() const { 
        std::lock_guard<std::mutex> lock(m_mutex);
        return m_currentArtist; 
    }
    void setCurrentArtist(const std::string& artist) { 
        {
            std::lock_guard<std::mutex> lock(m_mutex);
            m_currentArtist = artist;
        }
        if (m_metadataChangeCallback) m_metadataChangeCallback();
    }

private:
    mutable std::mutex m_mutex;
    PlaybackState m_state = PlaybackState::STOPPED;
    int m_currentPositionSeconds = 0;
    int m_totalDurationSeconds = 0;
    // Track Metadata
    std::string m_currentTitle;
    std::string m_currentArtist;
    int m_volume = 70;
    std::string m_currentFilePath;
    MediaType m_currentMediaType = MediaType::UNKNOWN;
    
    // Video-specific
    bool m_isFullscreen = false;
    double m_aspectRatio = 16.0 / 9.0;
    
    // Callbacks
    StateChangeCallback m_stateChangeCallback;
    MetadataChangeCallback m_metadataChangeCallback;
};

} // namespace models
} // namespace media_player

#endif // PLAYBACK_STATE_MODEL_H