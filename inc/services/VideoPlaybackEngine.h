#ifndef VIDEO_PLAYBACK_ENGINE_H
#define VIDEO_PLAYBACK_ENGINE_H

// System includes
#include <string>
#include <atomic>
#include <thread>
#include <memory>
#include <mutex>
#include <condition_variable>

// SDL2 includes
#include <SDL2/SDL.h>

// FFmpeg includes
extern "C" 
{
#include <libavformat/avformat.h>
#include <libavcodec/avcodec.h>
#include <libavutil/imgutils.h>
#include <libswscale/swscale.h>
#include <libswresample/swresample.h>
}

// Project includes
#include "IPlaybackEngine.h"
#include "utils/ThreadSafeQueue.h"
#include "utils/AVSyncClock.h"

namespace media_player 
{
namespace services 
{

// Frame structures for queuing
struct VideoFrame 
{
    AVFrame* frame;
    double pts;
    
    VideoFrame() : frame(nullptr), pts(0.0) {}
    ~VideoFrame() 
    {
        if (frame) 
        {
            av_frame_free(&frame);
        }
    }
};

struct AudioFrame 
{
    uint8_t* data;
    int size;
    double pts;
    
    AudioFrame() : data(nullptr), size(0), pts(0.0) {}
    ~AudioFrame() 
    {
        if (data) 
        {
            av_free(data);
        }
    }
};

class VideoPlaybackEngine : public IPlaybackEngine 
{
public:
    VideoPlaybackEngine();
    ~VideoPlaybackEngine();
    
    // IPlaybackEngine interface implementation
    bool loadFile(const std::string& filePath) override;
    bool play() override;
    bool pause() override;
    bool stop() override;
    bool seek(int positionSeconds) override;
    
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
    
    // Video-specific methods
    SDL_Texture* getCurrentVideoTexture();
    void getVideoResolution(int& width, int& height) const;
    double getFrameRate() const;
    
    // Render video frame từ main thread (SDL rendering phải từ main thread)
    void presentVideoFrame();
    
    // Update texture từ main thread (SDL operations phải từ main thread)
    void updateTextureFromMainThread();
    
    // Thiết lập renderer từ bên ngoài (main UI) thay vì tạo window riêng
    void setExternalRenderer(SDL_Renderer* renderer);
    
    // Kiểm tra có đang sử dụng external renderer không
    bool isUsingExternalRenderer() const { return m_useExternalRenderer; }
    
    // Kiểm tra có frame mới cần update không
    bool hasNewFrame() const { return m_frameReady.load(); }
    
private:
    // Initialization
    bool initializeFFmpeg();
    bool initializeSDL();
    void cleanup();
    
    // File loading
    bool openMediaFile(const std::string& filePath);
    bool findStreamIndices();
    bool openCodecs();
    void closeMediaFile();
    
    // Threading
    void demuxThread();
    void videoDecodeThread();
    void audioDecodeThread();
    void videoRenderThread();
    void audioPlaybackThread();
    void updateThread();
    
    // Decoding
    bool decodeVideoPacket(AVPacket* packet);
    bool decodeAudioPacket(AVPacket* packet);
    
    // Rendering
    void renderVideoFrame(VideoFrame* frame);
    void updateVideoTexture(AVFrame* frame);
    
    // Audio callback
    static void audioCallback(void* userdata, Uint8* stream, int len);
    void fillAudioBuffer(Uint8* stream, int len);
    
    // Synchronization
    double getAudioClock() const;
    double getVideoClock() const;
    double getMasterClock() const;
    void synchronizeVideo(VideoFrame* frame);
    
    // Seeking
    void performSeek(int targetSeconds);
    void flushQueues();
    bool isPlaybackDrained() const;
    
    // Notifications
    void notifyStateChange(PlaybackState newState);
    void notifyPosition();
    void notifyError(const std::string& error);
    void notifyFinished();
    
    // FFmpeg context
    AVFormatContext* m_formatContext;
    AVCodecContext* m_videoCodecContext;
    AVCodecContext* m_audioCodecContext;
    
    int m_videoStreamIndex;
    int m_audioStreamIndex;
    
    SwsContext* m_swsContext;
    SwrContext* m_swrContext;
    
    // SDL context
    bool m_sdlInitialized;
    SDL_Window* m_window;
    SDL_Renderer* m_renderer;
    SDL_Texture* m_texture;
    SDL_AudioDeviceID m_audioDevice;
    SDL_AudioSpec m_audioSpec;
    
    // External renderer (từ main UI)
    bool m_useExternalRenderer = false;
    SDL_Renderer* m_externalRenderer = nullptr;
    
    // Frame queues
    utils::ThreadSafeQueue<VideoFrame*> m_videoFrameQueue;
    utils::ThreadSafeQueue<AudioFrame*> m_audioFrameQueue;
    utils::ThreadSafeQueue<AVPacket*> m_videoPacketQueue;
    utils::ThreadSafeQueue<AVPacket*> m_audioPacketQueue;
    
    // Synchronization
    utils::AVSyncClock m_syncClock;
    std::atomic<double> m_audioClock;
    std::atomic<double> m_videoClock;
    
    // State
    std::atomic<PlaybackState> m_state;
    std::atomic<int> m_volume;
    std::atomic<int> m_currentPositionSeconds;
    std::atomic<int> m_totalDurationSeconds;
    
    std::string m_currentFilePath;
    int m_videoWidth;
    int m_videoHeight;
    double m_frameRate;
    
    // Threading
    std::atomic<bool> m_shouldStop;
    std::atomic<bool> m_seekRequested;
    std::atomic<int> m_seekTarget;
    std::atomic<bool> m_endOfStreamReached;
    std::atomic<bool> m_finishedEventQueued;
    
    std::unique_ptr<std::thread> m_demuxThread;
    std::unique_ptr<std::thread> m_videoDecodeThread;
    std::unique_ptr<std::thread> m_audioDecodeThread;
    std::unique_ptr<std::thread> m_videoRenderThread;
    std::unique_ptr<std::thread> m_updateThread;
    
    mutable std::mutex m_mutex;
    mutable std::mutex m_textureMutex;
    std::condition_variable m_frameAvailableCV;
    
    // Audio buffer for playback
    std::vector<uint8_t> m_audioBuffer;
    size_t m_audioBufferIndex;
    mutable std::mutex m_audioMutex;
    
    // Video frame buffer cho main thread update texture
    std::vector<uint8_t> m_yPlane;
    std::vector<uint8_t> m_uPlane;
    std::vector<uint8_t> m_vPlane;
    int m_yPitch = 0;
    int m_uPitch = 0;
    int m_vPitch = 0;
    std::atomic<bool> m_frameReady{false};  // Flag báo có frame mới cần update
    
    // Callbacks
    PlaybackStateChangeCallback m_stateChangeCallback;
    PlaybackPositionCallback m_positionCallback;
    PlaybackErrorCallback m_errorCallback;
    PlaybackFinishedCallback m_finishedCallback;
};

} // namespace services
} // namespace media_player

#endif // VIDEO_PLAYBACK_ENGINE_H