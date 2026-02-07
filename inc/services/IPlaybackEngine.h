#ifndef I_PLAYBACK_ENGINE_H
#define I_PLAYBACK_ENGINE_H

// System includes
#include <string>
#include <functional>

// Project includes
#include "models/MediaFileModel.h"

namespace media_player 
{
namespace services 
{

enum class PlaybackState 
{
    STOPPED,
    PLAYING,
    PAUSED
};

// Callbacks
using PlaybackStateChangeCallback = std::function<void(PlaybackState newState)>;
using PlaybackPositionCallback = std::function<void(int currentSeconds, int totalSeconds)>;
using PlaybackErrorCallback = std::function<void(const std::string& error)>;
using PlaybackFinishedCallback = std::function<void()>;

class IPlaybackEngine 
{
public:
    virtual ~IPlaybackEngine() = default;
    
    // Playback control
    virtual bool loadFile(const std::string& filePath) = 0;
    virtual bool play() = 0;
    virtual bool pause() = 0;
    virtual bool stop() = 0;
    virtual bool seek(int positionSeconds) = 0;
    
    // Resource management
    virtual void releaseResources() {}
    
    // State queries
    virtual PlaybackState getState() const = 0;
    virtual int getCurrentPosition() const = 0;
    virtual int getTotalDuration() const = 0;
    
    bool isPlaying() const 
    { 
        return getState() == PlaybackState::PLAYING; 
    }
    
    // Volume control (0-100)
    virtual void setVolume(int volume) = 0;
    virtual int getVolume() const = 0;
    
    // Media type support
    virtual bool supportsMediaType(models::MediaType type) const = 0;
    
    // Callbacks
    virtual void setStateChangeCallback(PlaybackStateChangeCallback callback) = 0;
    virtual void setPositionCallback(PlaybackPositionCallback callback) = 0;
    virtual void setErrorCallback(PlaybackErrorCallback callback) = 0;
    virtual void setFinishedCallback(PlaybackFinishedCallback callback) = 0;
};

} // namespace services
} // namespace media_player

#endif // I_PLAYBACK_ENGINE_H