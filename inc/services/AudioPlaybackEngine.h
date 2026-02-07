#ifndef AUDIO_PLAYBACK_ENGINE_H
#define AUDIO_PLAYBACK_ENGINE_H

// System includes
#include <string>
#include <vector>
#include <mutex>
#include <atomic>
#include <thread>
#include <functional>

// SDL2 includes
#include <SDL2/SDL.h>
#include <SDL2/SDL_mixer.h>

// Project includes
#include "IPlaybackEngine.h"

namespace media_player 
{
namespace services 
{

class AudioPlaybackEngine : public IPlaybackEngine 
{
public:
    AudioPlaybackEngine();
    ~AudioPlaybackEngine();
    
    // IPlaybackEngine interface implementation
    bool loadFile(const std::string& filePath) override;
    bool play() override;
    bool pause() override;
    bool stop() override;
    bool seek(int positionSeconds) override;
    void releaseResources() override;
    
    PlaybackState getState() const override;
    int getCurrentPosition() const override;
    int getTotalDuration() const override;
    
    void setVolume(int volume) override;
    int getVolume() const override;
    
    bool supportsMediaType(models::MediaType type) const override;
    
    void setStateChangeCallback(PlaybackStateChangeCallback callback) override;
    void setPositionCallback(PlaybackPositionCallback callback) override;
    void setErrorCallback(PlaybackErrorCallback callback) override;
    void setFinishedCallback(PlaybackFinishedCallback callback) override;
    
private:
    // Initialization
    bool initializeSDL();
    void cleanupSDL();
    void unloadAudioFile();
    
    // Threading
    void updateThread();
    
    // Notifications
    void notifyStateChange(PlaybackState newState);
    void notifyPosition();
    void notifyError(const std::string& error);
    void notifyFinished();
    
    // Callback for mixer
    static void musicFinishedCallback();
    void onMusicFinished();
    
    // SDL Mixer resources
    Mix_Music* m_music;
    bool m_sdlInitialized;
    
    // Threading - khai báo trước để khởi tạo đúng thứ tự
    std::atomic<bool> m_shouldStop;
    
    // Thread safety for callback
    std::atomic<bool> m_musicFinished{false};
    std::atomic<bool> m_manualStop{false};  // Flag to prevent notifyFinished on manual stop
    
    // State
    std::atomic<PlaybackState> m_state;
    std::atomic<int> m_volume;
    std::atomic<int> m_currentPositionSeconds;
    std::atomic<int> m_totalDurationSeconds;
    
    std::string m_currentFilePath;
    std::unique_ptr<std::thread> m_updateThread;
    
    mutable std::mutex m_mutex;
    
    // Callbacks
    PlaybackStateChangeCallback m_stateChangeCallback;
    PlaybackPositionCallback m_positionCallback;
    PlaybackErrorCallback m_errorCallback;
    PlaybackFinishedCallback m_finishedCallback;
    
    // Static instance for callback
    static AudioPlaybackEngine* s_instance;
};

} // namespace services
} // namespace media_player

#endif // AUDIO_PLAYBACK_ENGINE_H