// Project includes
#include "services/AudioPlaybackEngine.h"
#include "utils/Logger.h"
#include "config/AppConfig.h"

// System includes
#include <chrono>

namespace media_player 
{
namespace services 
{

AudioPlaybackEngine* AudioPlaybackEngine::s_instance = nullptr;

void AudioPlaybackEngine::musicFinishedCallback()
{
    if (s_instance)
    {
        s_instance->onMusicFinished();
    }
}

AudioPlaybackEngine::AudioPlaybackEngine()
    : m_music(nullptr)
    , m_sdlInitialized(false)
    , m_shouldStop(false)
    , m_musicFinished(false)
    , m_state(PlaybackState::STOPPED)
    , m_volume(config::AppConfig::DEFAULT_VOLUME)
    , m_currentPositionSeconds(0)
    , m_totalDurationSeconds(0)
{
    s_instance = this;
    
    if (!initializeSDL()) 
    {
        LOG_ERROR("Failed to initialize SDL/Mixer for audio playback");
        return;
    }
    
    m_shouldStop = false;
    m_updateThread = std::make_unique<std::thread>(&AudioPlaybackEngine::updateThread, this);
    
    LOG_INFO("AudioPlaybackEngine initialized successfully");
}

AudioPlaybackEngine::~AudioPlaybackEngine() 
{
    stop();
    
    m_shouldStop = true;
    
    if (m_updateThread && m_updateThread->joinable()) 
    {
        m_updateThread->join();
    }
    
    unloadAudioFile();
    cleanupSDL();
    
    s_instance = nullptr;
    
    LOG_INFO("AudioPlaybackEngine destroyed");
}

bool AudioPlaybackEngine::initializeSDL() 
{
    if (SDL_Init(SDL_INIT_AUDIO) < 0) 
    {
        LOG_ERROR("SDL_Init failed: " + std::string(SDL_GetError()));
        return false;
    }
    
    // Initialize SDL_mixer for MP3, OGG, FLAC support
    int flags = MIX_INIT_MP3 | MIX_INIT_OGG | MIX_INIT_FLAC;
    int initialized = Mix_Init(flags);
    
    if ((initialized & flags) != flags) 
    {
        LOG_WARNING("Mix_Init: NOT all requested formats initialized. Missing: " + 
                   std::to_string(flags ^ (initialized & flags)));
        LOG_WARNING("Mix_Init error: " + std::string(Mix_GetError()));
        // Don't fail completely, try to continue
    }
    
    // We don't open audio here yet, we open it on play/load to play nice with VideoEngine
    // But Mix_LoadMUS might require open audio? No, usually not. 
    // Wait, documentation says "You must call Mix_OpenAudio() before using any other functions in this library."
    // Let's open it here, and if needed we can close it in stop().
    // Update: opening/closing repeatedly might be buggy in some SDL versions.
    // Ideally we keep it open. But we have VideoEngine which uses SDL_OpenAudioDevice.
    // SDL_mixer uses SDL_OpenAudio. They CONFLICT. 
    // We MUST open/close on demand or when switching engines.
    
    m_sdlInitialized = true;
    LOG_INFO("SDL Audio & Mixer initialized successfully");
    
    return true;
}

void AudioPlaybackEngine::cleanupSDL() 
{
    // Ensure device is closed
    Mix_CloseAudio();
    
    if (m_sdlInitialized) 
    {
        Mix_Quit();
        SDL_Quit();
        m_sdlInitialized = false;
    }
}

void AudioPlaybackEngine::unloadAudioFile() 
{
    if (m_music) 
    {
        Mix_FreeMusic(m_music);
        m_music = nullptr;
    }
    
    m_currentPositionSeconds = 0;
    m_totalDurationSeconds = 0;
    m_currentFilePath.clear();
}

bool AudioPlaybackEngine::loadFile(const std::string& filePath) 
{
    std::lock_guard<std::mutex> lock(m_mutex);
    
    LOG_INFO("Loading audio file: " + filePath);
    
    try
    {
        if (m_state != PlaybackState::STOPPED) 
        {
            stop();
        }
        
        unloadAudioFile();
        
        // Kiểm tra file tồn tại
        if (filePath.empty())
        {
            LOG_ERROR("Empty file path");
            notifyError("Empty file path");
            return false;
        }
        
        // Check if audio is already open (query logic or just try open)
        // Mix_OpenAudio can be called multiple times safely in some versions, but better to check
        int frequency = 44100;
        Uint16 format = MIX_DEFAULT_FORMAT;
        int channels = 2;
        int queryResult = Mix_QuerySpec(&frequency, &format, &channels);
        
        if (queryResult == 0) // Not open
        {
            // Use 4096 sample buffer (approx 92ms latency) to prevent stuttering under load
            if (Mix_OpenAudio(44100, MIX_DEFAULT_FORMAT, 2, 4096) < 0)
            {
                LOG_ERROR("Mix_OpenAudio failed: " + std::string(Mix_GetError()));
                notifyError("Failed to open audio device");
                return false;
            }
        }
        
        m_music = Mix_LoadMUS(filePath.c_str());
        
        if (!m_music) 
        {
            std::string error = Mix_GetError();
            LOG_ERROR("Mix_LoadMUS failed: " + error);
            notifyError("Unsupported audio format: " + error);
            Mix_CloseAudio();
            return false;
        }
        
        // Set hook for music finished
        Mix_HookMusicFinished(AudioPlaybackEngine::musicFinishedCallback);
        
        m_currentFilePath = filePath;
        m_state = PlaybackState::STOPPED;
        m_currentPositionSeconds = 0;
        
        // Estimate generic duration since Mix_LoadMUS doesn't direct give it for streams easily.
        // For MP3s Mix_MusicDuration might work in newer SDL_mixer versions logic or we need TagLib.
        // We already use TagLib in MetadataReader, so duration should come from there via Playlist/Library model?
        // But PlaybackController expects valid duration here?
        // Let's rely on MetadataReader previously read duration or file length approximation if possible.
        // Since we don't have easy access to MetadataReader here without dependency refactor, 
        // we can use Mix_MusicDuration if available (SDL2_mixer 2.6.0+).
        // Assuming standard environment. If it fails we report 0 and UI handles it.
        
        double duration = Mix_MusicDuration(m_music);
        if (duration > 0)
        {
            m_totalDurationSeconds = static_cast<int>(duration);
        }
        else
        {
            // Fallback or leave as 0 (controller might fill it from metadata?)
            // The PlaybackController calls onPositionChanged. 
            // If we report 0 total, the UI progress bar behaves oddly but doesn't crash.
            // Usually currentDuration is updated by UI from MediaModel, checking...
            // UI uses m_state.playbackDuration which comes from PlaybackController.
            m_totalDurationSeconds = 0;
        }
        
        LOG_INFO("Audio file loaded successfully");
        notifyStateChange(PlaybackState::STOPPED);
        
        return true;
    }
    catch (const std::exception& e)
    {
        LOG_ERROR("Exception in loadFile: " + std::string(e.what()));
        notifyError("Load error: " + std::string(e.what()));
        unloadAudioFile();
        return false;
    }
    catch (...)
    {
        LOG_ERROR("Unknown exception in loadFile");
        notifyError("Unknown load error");
        unloadAudioFile();
        return false;
    }
}

bool AudioPlaybackEngine::play() 
{
    std::lock_guard<std::mutex> lock(m_mutex);
    
    try
    {
        if (!m_music) 
        {
            LOG_WARNING("No audio file loaded");
            return false;
        }
        
        if (m_state == PlaybackState::PLAYING) 
        {
            return true;
        }
        
        // Reset manual stop flag so natural finish will trigger notifyFinished
        m_manualStop = false;
        
        if (m_state == PlaybackState::PAUSED)
        {
            Mix_ResumeMusic();
        }
        else
        {
            // Play from start
            if (Mix_PlayMusic(m_music, 0) == -1) 
            {
                std::string error = Mix_GetError();
                LOG_ERROR("Mix_PlayMusic failed: " + error);
                notifyError("Playback error: " + error);
                return false;
            }
        }
        
        Mix_VolumeMusic(m_volume * 128 / 100); // SDL_mixer volume is 0-128
        
        m_state = PlaybackState::PLAYING;
        
        LOG_INFO("Audio playback started");
        notifyStateChange(PlaybackState::PLAYING);
        
        return true;
    }
    catch (const std::exception& e)
    {
        LOG_ERROR("Exception in play: " + std::string(e.what()));
        notifyError("Playback error: " + std::string(e.what()));
        return false;
    }
    catch (...)
    {
        LOG_ERROR("Unknown exception in play");
        notifyError("Unknown playback error");
        return false;
    }
}

bool AudioPlaybackEngine::pause() 
{
    std::lock_guard<std::mutex> lock(m_mutex);
    
    if (m_state != PlaybackState::PLAYING) 
    {
        return false;
    }
    
    Mix_PauseMusic();
    m_state = PlaybackState::PAUSED;
    
    LOG_INFO("Audio playback paused");
    notifyStateChange(PlaybackState::PAUSED);
    
    return true;
}

bool AudioPlaybackEngine::stop() 
{
    std::lock_guard<std::mutex> lock(m_mutex);
    
    if (m_state == PlaybackState::STOPPED) 
    {
        return true;
    }
    
    // Set manual stop flag BEFORE Mix_HaltMusic to prevent race condition
    // This tells updateThread not to call notifyFinished()
    m_manualStop = true;
    
    Mix_HaltMusic();
    
    // Clear the musicFinished flag in case it was set
    m_musicFinished = false;
    
    // DO NOT Close audio device here to avoid rapid open/close cycles
    // Device will be closed via releaseResources when switching to Video
    
    m_state = PlaybackState::STOPPED;
    m_currentPositionSeconds = 0;
    
    LOG_INFO("Audio playback stopped");
    notifyStateChange(PlaybackState::STOPPED);
    
    return true;
}

void AudioPlaybackEngine::releaseResources()
{
    std::lock_guard<std::mutex> lock(m_mutex);
    if (m_music)
    {
        Mix_FreeMusic(m_music);
        m_music = nullptr;
    }
    Mix_CloseAudio();
    LOG_INFO("Audio device closed");
}

bool AudioPlaybackEngine::seek(int positionSeconds)
{
    std::lock_guard<std::mutex> lock(m_mutex);
    
    if (!m_music) return false;
    
    if (Mix_SetMusicPosition(static_cast<double>(positionSeconds)) == 0)
    {
        m_currentPositionSeconds = positionSeconds;
        notifyPosition();
        return true;
    }
    
    return false;
}

PlaybackState AudioPlaybackEngine::getState() const
{
    return m_state;
}

int AudioPlaybackEngine::getCurrentPosition() const
{
    return m_currentPositionSeconds;
}

int AudioPlaybackEngine::getTotalDuration() const
{
    return m_totalDurationSeconds;
}

void AudioPlaybackEngine::setVolume(int volume)
{
    std::lock_guard<std::mutex> lock(m_mutex);
    m_volume = volume;
    if (m_music)
    {
        Mix_VolumeMusic(m_volume * 128 / 100);
    }
}

int AudioPlaybackEngine::getVolume() const
{
    return m_volume;
}

bool AudioPlaybackEngine::supportsMediaType(models::MediaType type) const
{
    return type == models::MediaType::AUDIO;
}

void AudioPlaybackEngine::setStateChangeCallback(PlaybackStateChangeCallback callback)
{
    m_stateChangeCallback = callback;
}

void AudioPlaybackEngine::setPositionCallback(PlaybackPositionCallback callback)
{
    m_positionCallback = callback;
}

void AudioPlaybackEngine::setErrorCallback(PlaybackErrorCallback callback)
{
    m_errorCallback = callback;
}

void AudioPlaybackEngine::setFinishedCallback(PlaybackFinishedCallback callback)
{
    m_finishedCallback = callback;
}



void AudioPlaybackEngine::onMusicFinished()
{
    // Called on separate SDL mixer thread
    // DO NOT call into complex logic here or call Mix_ functions
    // Just set a flag for the update thread to handle
    m_musicFinished = true;
}

void AudioPlaybackEngine::updateThread() 
{
    try
    {
        while (!m_shouldStop) 
        {
            // Check for music finished event from mixer thread
            if (m_musicFinished)
            {
                m_musicFinished = false;
                m_state = PlaybackState::STOPPED;
                notifyStateChange(PlaybackState::STOPPED);
                
                // Only call notifyFinished if this was NOT a manual stop
                if (!m_manualStop)
                {
                    notifyFinished();
                }
                m_manualStop = false;  // Reset for next time
            }

            if (m_state == PlaybackState::PLAYING) 
            {
                // Lock mutex to safely access m_music
                {
                    std::lock_guard<std::mutex> lock(m_mutex);
                    
                    // Verify we are still playing and music exists under lock
                    if (m_state == PlaybackState::PLAYING && m_music)
                    {
                        // Mix_GetMusicPosition works in recent SDL_mixer versions.
                        double pos = Mix_GetMusicPosition(m_music);
                        if (pos >= 0)
                        {
                            m_currentPositionSeconds = static_cast<int>(pos);
                        }
                    }
                }
                // Unlock before notifying to avoid deadlock with Controller mutex

                notifyPosition();
            }
            
            std::this_thread::sleep_for(
                std::chrono::milliseconds(config::AppConfig::PLAYBACK_UPDATE_INTERVAL_MS)
            );
        }
    }
    catch (const std::exception& e)
    {
        LOG_ERROR("Audio update thread exception: " + std::string(e.what()));
    }
    catch (...)
    {
        LOG_ERROR("Audio update thread: Unknown exception");
    }
}

void AudioPlaybackEngine::notifyStateChange(PlaybackState newState) 
{
    if (m_stateChangeCallback) 
    {
        m_stateChangeCallback(newState);
    }
}

void AudioPlaybackEngine::notifyPosition() 
{
    if (m_positionCallback) 
    {
        m_positionCallback(m_currentPositionSeconds, m_totalDurationSeconds);
    }
}

void AudioPlaybackEngine::notifyError(const std::string& error) 
{
    if (m_errorCallback) 
    {
        m_errorCallback(error);
    }
}

void AudioPlaybackEngine::notifyFinished() 
{
    if (m_finishedCallback) 
    {
        m_finishedCallback();
    }
}

} // namespace services
} // namespace media_player