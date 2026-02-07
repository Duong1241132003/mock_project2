// Project includes
#include "services/VideoPlaybackEngine.h"
#include "utils/Logger.h"
#include "config/AppConfig.h"
#include "core/Application.h"

// System includes
#include <chrono>

namespace media_player 
{
namespace services 
{

VideoPlaybackEngine::VideoPlaybackEngine()
    : m_formatContext(nullptr)
    , m_videoCodecContext(nullptr)
    , m_audioCodecContext(nullptr)
    , m_videoStreamIndex(-1)
    , m_audioStreamIndex(-1)
    , m_swsContext(nullptr)
    , m_swrContext(nullptr)
    , m_sdlInitialized(false)
    , m_window(nullptr)
    , m_renderer(nullptr)
    , m_texture(nullptr)
    , m_audioDevice(0)
    , m_audioClock(0.0)
    , m_videoClock(0.0)
    , m_state(PlaybackState::STOPPED)
    , m_volume(config::AppConfig::DEFAULT_VOLUME)
    , m_currentPositionSeconds(0)
    , m_totalDurationSeconds(0)
    , m_videoWidth(0)
    , m_videoHeight(0)
    , m_frameRate(0.0)
    , m_shouldStop(false)
    , m_seekRequested(false)
    , m_seekTarget(0)
    , m_endOfStreamReached(false)
    , m_finishedEventQueued(false)
    , m_audioBufferIndex(0)
{
    if (!initializeFFmpeg()) 
    {
        LOG_ERROR("Failed to initialize FFmpeg");
        return;
    }
    
    if (!initializeSDL()) 
    {
        LOG_ERROR("Failed to initialize SDL for video playback");
        return;
    }
    
    LOG_INFO("VideoPlaybackEngine initialized successfully");
}

VideoPlaybackEngine::~VideoPlaybackEngine() 
{
    stop();
    cleanup();
    
    LOG_INFO("VideoPlaybackEngine destroyed");
}

bool VideoPlaybackEngine::initializeFFmpeg() 
{
    LOG_INFO("Initializing FFmpeg");
    return true;
}

bool VideoPlaybackEngine::initializeSDL() 
{
    // Không gọi SDL_Init ở đây vì Application đã init rồi
    // Việc gọi SDL_Init lần nữa có thể gây conflict
    m_sdlInitialized = true;
    LOG_INFO("VideoPlaybackEngine SDL ready (using Application's SDL context)");
    
    return true;
}

void VideoPlaybackEngine::cleanup() 
{
    closeMediaFile();
    
    if (m_texture) 
    {
        SDL_DestroyTexture(m_texture);
        m_texture = nullptr;
    }
    
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
    
    if (m_audioDevice != 0) 
    {
        SDL_CloseAudioDevice(m_audioDevice);
        m_audioDevice = 0;
    }
    
    // Không gọi SDL_Quit() ở đây vì Application quản lý SDL lifecycle
    m_sdlInitialized = false;
}

bool VideoPlaybackEngine::loadFile(const std::string& filePath) 
{
    std::lock_guard<std::mutex> lock(m_mutex);
    
    LOG_INFO("Loading video file: " + filePath);
    
    try 
    {
        if (m_state != PlaybackState::STOPPED) 
        {
            stop();
        }
        
        closeMediaFile();
        
        if (!openMediaFile(filePath)) 
        {
            LOG_ERROR("Failed to open media file: " + filePath);
            notifyError("Failed to load file");
            return false;
        }
        
        if (!findStreamIndices()) 
        {
            LOG_ERROR("Failed to find stream indices");
            closeMediaFile();
            notifyError("Invalid stream info");
            return false;
        }
        
        if (!openCodecs()) 
        {
            LOG_ERROR("Failed to open codecs");
            closeMediaFile();
            notifyError("Codec error");
            return false;
        }
        
        // Chọn renderer để sử dụng
        SDL_Renderer* rendererToUse = nullptr;
        
        if (m_useExternalRenderer && m_externalRenderer)
        {
            // Sử dụng external renderer từ main UI
            rendererToUse = m_externalRenderer;
            LOG_INFO("Using external renderer for video texture");
        }
        else
        {
            // Tạo window và renderer riêng (fallback)
            if (!m_window) 
            {
                 m_window = SDL_CreateWindow(
                    "Video Player",
                    SDL_WINDOWPOS_CENTERED,
                    SDL_WINDOWPOS_CENTERED,
                    m_videoWidth,
                    m_videoHeight,
                    SDL_WINDOW_SHOWN | SDL_WINDOW_RESIZABLE
                );
            }
            else 
            {
                SDL_SetWindowSize(m_window, m_videoWidth, m_videoHeight);
                SDL_ShowWindow(m_window);
            }
            
            if (!m_window) 
            {
                LOG_ERROR("Failed to create SDL window: " + std::string(SDL_GetError()));
                closeMediaFile();
                return false;
            }
            
            if (!m_renderer) 
            {
                 m_renderer = SDL_CreateRenderer(m_window, -1, SDL_RENDERER_ACCELERATED);
            }
            
            if (!m_renderer) 
            {
                LOG_ERROR("Failed to create SDL renderer: " + std::string(SDL_GetError()));
                SDL_DestroyWindow(m_window);
                m_window = nullptr;
                closeMediaFile();
                return false;
            }
            
            rendererToUse = m_renderer;
        }
        
        // Tạo texture cho video
        if (m_texture) 
        {
            SDL_DestroyTexture(m_texture);
        }
        
        m_texture = SDL_CreateTexture(
            rendererToUse,
            SDL_PIXELFORMAT_YV12,
            SDL_TEXTUREACCESS_STREAMING,
            m_videoWidth,
            m_videoHeight
        );
        
        if (!m_texture) 
        {
            LOG_ERROR("Failed to create SDL texture: " + std::string(SDL_GetError()));
            closeMediaFile();
            return false;
        }
        
        // Setup audio device
        if (m_audioStreamIndex != -1 && m_audioCodecContext) 
        {
            SDL_AudioSpec desiredSpec;
            SDL_zero(desiredSpec);
            desiredSpec.freq = m_audioCodecContext->sample_rate;
            desiredSpec.format = AUDIO_S16SYS;
            desiredSpec.channels = m_audioCodecContext->ch_layout.nb_channels;
            desiredSpec.silence = 0;
            desiredSpec.samples = 1024;
            desiredSpec.callback = audioCallback;
            desiredSpec.userdata = this;
            
            if (m_audioDevice != 0) 
            {
                SDL_CloseAudioDevice(m_audioDevice);
            }
            
            m_audioDevice = SDL_OpenAudioDevice(nullptr, 0, &desiredSpec, &m_audioSpec, 0);
            
            if (m_audioDevice == 0) 
            {
                LOG_WARNING("Failed to open audio device: " + std::string(SDL_GetError()) + ". Continuing without audio.");
            }
        }
        
        m_currentFilePath = filePath;
        m_state = PlaybackState::STOPPED;
        m_currentPositionSeconds = 0;
        m_endOfStreamReached = false;
        m_finishedEventQueued = false;
        
        LOG_INFO("Video file loaded successfully");
        notifyStateChange(PlaybackState::STOPPED);
        
        return true;
    }
    catch (const std::exception& e) 
    {
        LOG_ERROR("Exception in loadFile: " + std::string(e.what()));
        closeMediaFile();
        return false;
    }
    catch (...) 
    {
        LOG_ERROR("Unknown exception in loadFile");
        closeMediaFile();
        return false;
    }
}

bool VideoPlaybackEngine::isPlaybackDrained() const
{
    // Đảm bảo tất cả queue và buffer đã được xử lý xong trước khi phát sự kiện finished
    const bool videoPacketsEmpty = m_videoPacketQueue.empty();
    const bool audioPacketsEmpty = m_audioPacketQueue.empty();
    const bool videoFramesEmpty = m_videoFrameQueue.empty();
    const bool audioFramesEmpty = m_audioFrameQueue.empty();
    bool audioBufferDrained = true;

    if (m_audioStreamIndex != -1)
    {
        std::lock_guard<std::mutex> lock(m_audioMutex);
        audioBufferDrained = m_audioBufferIndex >= m_audioBuffer.size();
    }

    return videoPacketsEmpty && audioPacketsEmpty && videoFramesEmpty && audioFramesEmpty && audioBufferDrained;
}

bool VideoPlaybackEngine::openMediaFile(const std::string& filePath) 
{
    m_formatContext = avformat_alloc_context();
    
    if (avformat_open_input(&m_formatContext, filePath.c_str(), nullptr, nullptr) != 0) 
    {
        LOG_ERROR("Could not open media file: " + filePath);
        return false;
    }
    
    if (avformat_find_stream_info(m_formatContext, nullptr) < 0) 
    {
        LOG_ERROR("Could not find stream info");
        return false;
    }
    
    // Get duration
    if (m_formatContext->duration != AV_NOPTS_VALUE) 
    {
        m_totalDurationSeconds = m_formatContext->duration / AV_TIME_BASE;
    }
    
    return true;
}

bool VideoPlaybackEngine::findStreamIndices() 
{
    m_videoStreamIndex = -1;
    m_audioStreamIndex = -1;
    
    for (unsigned int i = 0; i < m_formatContext->nb_streams; i++) 
    {
        AVCodecParameters* codecParams = m_formatContext->streams[i]->codecpar;
        
        if (codecParams->codec_type == AVMEDIA_TYPE_VIDEO && m_videoStreamIndex == -1) 
        {
            m_videoStreamIndex = i;
        }
        else if (codecParams->codec_type == AVMEDIA_TYPE_AUDIO && m_audioStreamIndex == -1) 
        {
            m_audioStreamIndex = i;
        }
    }
    
    if (m_videoStreamIndex == -1) 
    {
        LOG_ERROR("No video stream found");
        return false;
    }
    
    LOG_INFO("Video stream index: " + std::to_string(m_videoStreamIndex));
    
    if (m_audioStreamIndex != -1) 
    {
        LOG_INFO("Audio stream index: " + std::to_string(m_audioStreamIndex));
    }
    else 
    {
        LOG_WARNING("No audio stream found");
    }
    
    return true;
}

bool VideoPlaybackEngine::openCodecs() 
{
    // Open video codec
    AVCodecParameters* videoCodecParams = m_formatContext->streams[m_videoStreamIndex]->codecpar;
    const AVCodec* videoCodec = avcodec_find_decoder(videoCodecParams->codec_id);
    
    if (!videoCodec) 
    {
        LOG_ERROR("Video codec not found");
        return false;
    }
    
    m_videoCodecContext = avcodec_alloc_context3(videoCodec);
    avcodec_parameters_to_context(m_videoCodecContext, videoCodecParams);
    
    if (avcodec_open2(m_videoCodecContext, videoCodec, nullptr) < 0) 
    {
        LOG_ERROR("Could not open video codec");
        return false;
    }
    
    m_videoWidth = m_videoCodecContext->width;
    m_videoHeight = m_videoCodecContext->height;
    
    // Frame rate
    AVRational frameRate = m_formatContext->streams[m_videoStreamIndex]->avg_frame_rate;
    if (frameRate.den != 0) 
    {
        m_frameRate = static_cast<double>(frameRate.num) / static_cast<double>(frameRate.den);
    }
    
    LOG_INFO("Video resolution: " + std::to_string(m_videoWidth) + "x" + std::to_string(m_videoHeight));
    LOG_INFO("Frame rate: " + std::to_string(m_frameRate));
    
    // Open audio codec if available
    if (m_audioStreamIndex != -1) 
    {
        AVCodecParameters* audioCodecParams = m_formatContext->streams[m_audioStreamIndex]->codecpar;
        const AVCodec* audioCodec = avcodec_find_decoder(audioCodecParams->codec_id);
        
        if (!audioCodec) 
        {
            LOG_ERROR("Audio codec not found");
            return false;
        }
        
        m_audioCodecContext = avcodec_alloc_context3(audioCodec);
        avcodec_parameters_to_context(m_audioCodecContext, audioCodecParams);
        
        if (avcodec_open2(m_audioCodecContext, audioCodec, nullptr) < 0) 
        {
            LOG_ERROR("Could not open audio codec");
            return false;
        }
        
        LOG_INFO("Audio codec opened successfully");
    }
    
    // Initialize SWS context for video scaling/conversion
    m_swsContext = sws_getContext(
        m_videoWidth,
        m_videoHeight,
        m_videoCodecContext->pix_fmt,
        m_videoWidth,
        m_videoHeight,
        AV_PIX_FMT_YUV420P,
        SWS_BILINEAR,
        nullptr,
        nullptr,
        nullptr
    );
    
    if (!m_swsContext) 
    {
        LOG_ERROR("Could not initialize SWS context");
        return false;
    }
    
    return true;
}

void VideoPlaybackEngine::closeMediaFile() 
{
    if (m_swsContext) 
    {
        sws_freeContext(m_swsContext);
        m_swsContext = nullptr;
    }
    
    if (m_swrContext) 
    {
        swr_free(&m_swrContext);
        m_swrContext = nullptr;
    }
    
    if (m_videoCodecContext) 
    {
        avcodec_free_context(&m_videoCodecContext);
        m_videoCodecContext = nullptr;
    }
    
    if (m_audioCodecContext) 
    {
        avcodec_free_context(&m_audioCodecContext);
        m_audioCodecContext = nullptr;
    }
    
    if (m_formatContext) 
    {
        avformat_close_input(&m_formatContext);
        m_formatContext = nullptr;
    }
    
    flushQueues();
    m_endOfStreamReached = false;
    m_finishedEventQueued = false;
}

bool VideoPlaybackEngine::play() 
{
    std::lock_guard<std::mutex> lock(m_mutex);
    
    if (!m_formatContext) 
    {
        LOG_WARNING("No video file loaded");
        return false;
    }
    
    if (m_state == PlaybackState::PLAYING) 
    {
        LOG_DEBUG("Already playing");
        return true;
    }
    
    m_shouldStop = false;
    m_state = PlaybackState::PLAYING;
    m_endOfStreamReached = false;
    m_finishedEventQueued = false;
    
    // Reset các queue để có thể sử dụng lại sau khi stop
    m_videoPacketQueue.reset();
    m_audioPacketQueue.reset();
    m_videoFrameQueue.reset();
    m_audioFrameQueue.reset();
    
    // Start threads
    m_demuxThread = std::make_unique<std::thread>(&VideoPlaybackEngine::demuxThread, this);
    m_videoDecodeThread = std::make_unique<std::thread>(&VideoPlaybackEngine::videoDecodeThread, this);
    
    if (m_audioStreamIndex != -1) 
    {
        m_audioDecodeThread = std::make_unique<std::thread>(&VideoPlaybackEngine::audioDecodeThread, this);
    }
    
    m_videoRenderThread = std::make_unique<std::thread>(&VideoPlaybackEngine::videoRenderThread, this);
    m_updateThread = std::make_unique<std::thread>(&VideoPlaybackEngine::updateThread, this);
    
    // Start audio playback
    if (m_audioDevice != 0) 
    {
        SDL_PauseAudioDevice(m_audioDevice, 0);
    }
    
    // Start sync clock
    m_syncClock.start();
    
    LOG_INFO("Video playback started");
    notifyStateChange(PlaybackState::PLAYING);
    
    return true;
}

bool VideoPlaybackEngine::pause() 
{
    std::lock_guard<std::mutex> lock(m_mutex);
    
    if (m_state != PlaybackState::PLAYING) 
    {
        LOG_DEBUG("Not playing, cannot pause");
        return false;
    }
    
    m_state = PlaybackState::PAUSED;
    m_syncClock.pause();
    
    if (m_audioDevice != 0) 
    {
        SDL_PauseAudioDevice(m_audioDevice, 1);
    }
    
    LOG_INFO("Video playback paused");
    notifyStateChange(PlaybackState::PAUSED);
    
    return true;
}

bool VideoPlaybackEngine::stop() 
{
    std::lock_guard<std::mutex> lock(m_mutex);
    
    if (m_state == PlaybackState::STOPPED) 
    {
        return true;
    }
    
    m_shouldStop = true;
    m_state = PlaybackState::STOPPED;
    m_endOfStreamReached = false;
    m_finishedEventQueued = false;
    
    // Stop audio
    if (m_audioDevice != 0) 
    {
        SDL_PauseAudioDevice(m_audioDevice, 1);
    }
    
    // Dừng tất cả các queue để các thread đang chờ waitAndPop() có thể thoát
    m_videoPacketQueue.stop();
    m_audioPacketQueue.stop();
    m_videoFrameQueue.stop();
    m_audioFrameQueue.stop();
    
    // Wait for threads to finish
    if (m_demuxThread && m_demuxThread->joinable()) 
    {
        m_demuxThread->join();
    }
    
    if (m_videoDecodeThread && m_videoDecodeThread->joinable()) 
    {
        m_videoDecodeThread->join();
    }
    
    if (m_audioDecodeThread && m_audioDecodeThread->joinable()) 
    {
        m_audioDecodeThread->join();
    }
    
    if (m_videoRenderThread && m_videoRenderThread->joinable()) 
    {
        m_videoRenderThread->join();
    }
    
    if (m_updateThread && m_updateThread->joinable()) 
    {
        m_updateThread->join();
    }
    
    m_syncClock.reset();
    m_currentPositionSeconds = 0;
    
    flushQueues();
    
    LOG_INFO("Video playback stopped");
    notifyStateChange(PlaybackState::STOPPED);
    
    return true;
}

bool VideoPlaybackEngine::seek(int positionSeconds) 
{
    if (!m_formatContext) 
    {
        return false;
    }
    
    if (positionSeconds < 0 || positionSeconds > m_totalDurationSeconds) 
    {
        LOG_WARNING("Invalid seek position: " + std::to_string(positionSeconds));
        return false;
    }
    
    m_endOfStreamReached = false;
    m_finishedEventQueued = false;
    m_seekTarget = positionSeconds;
    m_seekRequested = true;
    
    LOG_INFO("Seek requested to position: " + std::to_string(positionSeconds) + "s");
    
    return true;
}

void VideoPlaybackEngine::performSeek(int targetSeconds) 
{
    int64_t timestamp = targetSeconds * AV_TIME_BASE;
    
    if (av_seek_frame(m_formatContext, -1, timestamp, AVSEEK_FLAG_BACKWARD) < 0) 
    {
        LOG_ERROR("Seek failed");
        return;
    }
    
    flushQueues();
    
    if (m_videoCodecContext) 
    {
        avcodec_flush_buffers(m_videoCodecContext);
    }
    
    if (m_audioCodecContext) 
    {
        avcodec_flush_buffers(m_audioCodecContext);
    }
    
    m_currentPositionSeconds = targetSeconds;
    m_syncClock.setMasterTime(targetSeconds);
    m_endOfStreamReached = false;
    m_finishedEventQueued = false;
    
    m_seekRequested = false;
    
    LOG_INFO("Seek completed");
    notifyPosition();
}

PlaybackState VideoPlaybackEngine::getState() const 
{
    return m_state;
}

int VideoPlaybackEngine::getCurrentPosition() const 
{
    return m_currentPositionSeconds;
}

int VideoPlaybackEngine::getTotalDuration() const 
{
    return m_totalDurationSeconds;
}

void VideoPlaybackEngine::setVolume(int volume) 
{
    if (volume < 0) volume = 0;
    if (volume > 100) volume = 100;
    
    m_volume = volume;
    
    LOG_DEBUG("Volume set to: " + std::to_string(volume));
}

int VideoPlaybackEngine::getVolume() const 
{
    return m_volume;
}

bool VideoPlaybackEngine::supportsMediaType(models::MediaType type) const 
{
    return type == models::MediaType::VIDEO;
}

void VideoPlaybackEngine::setStateChangeCallback(PlaybackStateChangeCallback callback) 
{
    m_stateChangeCallback = callback;
}

void VideoPlaybackEngine::setPositionCallback(PlaybackPositionCallback callback) 
{
    m_positionCallback = callback;
}

void VideoPlaybackEngine::setErrorCallback(PlaybackErrorCallback callback) 
{
    m_errorCallback = callback;
}

void VideoPlaybackEngine::setFinishedCallback(PlaybackFinishedCallback callback) 
{
    m_finishedCallback = callback;
}

SDL_Texture* VideoPlaybackEngine::getCurrentVideoTexture() 
{
    std::lock_guard<std::mutex> lock(m_textureMutex);
    return m_texture;
}

void VideoPlaybackEngine::getVideoResolution(int& width, int& height) const 
{
    width = m_videoWidth;
    height = m_videoHeight;
}

double VideoPlaybackEngine::getFrameRate() const 
{
    return m_frameRate;
}

void VideoPlaybackEngine::presentVideoFrame()
{
    // Render video frame từ main thread
    // SDL rendering operations phải được thực hiện từ main thread
    std::lock_guard<std::mutex> lock(m_textureMutex);
    
    // Nếu sử dụng external renderer, không cần present ở đây
    // Main UI sẽ lấy texture và render
    if (m_useExternalRenderer)
    {
        return;
    }
    
    if (!m_renderer || !m_texture)
    {
        return;
    }
    
    SDL_RenderClear(m_renderer);
    SDL_RenderCopy(m_renderer, m_texture, nullptr, nullptr);
    SDL_RenderPresent(m_renderer);
}

void VideoPlaybackEngine::setExternalRenderer(SDL_Renderer* renderer)
{
    std::lock_guard<std::mutex> lock(m_mutex);
    
    m_externalRenderer = renderer;
    m_useExternalRenderer = (renderer != nullptr);
    
    if (m_useExternalRenderer)
    {
        LOG_INFO("VideoPlaybackEngine: Using external renderer from main UI");
    }
    else
    {
        LOG_INFO("VideoPlaybackEngine: Using internal window/renderer");
    }
}

void VideoPlaybackEngine::updateTextureFromMainThread()
{
    // Update texture từ main thread - SDL operations phải từ main thread
    std::lock_guard<std::mutex> lock(m_textureMutex);
    
    if (!m_frameReady || !m_texture)
    {
        return;
    }
    
    if (m_yPlane.empty() || m_uPlane.empty() || m_vPlane.empty())
    {
        return;
    }
    
    // Update texture với frame data từ buffer
    SDL_UpdateYUVTexture(m_texture,
                         nullptr,
                         m_yPlane.data(),
                         m_yPitch,
                         m_uPlane.data(),
                         m_uPitch,
                         m_vPlane.data(),
                         m_vPitch);
    
    m_frameReady = false;
}

void VideoPlaybackEngine::demuxThread() 
{
    LOG_INFO("Demux thread started");
    
    try
    {
        // Kiểm tra format context hợp lệ
        if (!m_formatContext)
        {
            LOG_ERROR("Demux thread: Invalid format context");
            return;
        }
        
        AVPacket* packet = av_packet_alloc();
        if (!packet)
        {
            LOG_ERROR("Demux thread: Failed to allocate packet");
            return;
        }
        
        while (!m_shouldStop) 
        {
            // Kiểm tra format context còn hợp lệ
            if (!m_formatContext)
            {
                LOG_WARNING("Demux thread: Format context became invalid");
                break;
            }
            
            if (m_seekRequested) 
            {
                performSeek(m_seekTarget);
            }
            
            int readResult = av_read_frame(m_formatContext, packet);
            if (readResult >= 0) 
            {
                if (packet->stream_index == m_videoStreamIndex) 
                {
                    AVPacket* videoPkt = av_packet_clone(packet);
                    if (videoPkt)
                    {
                        m_videoPacketQueue.push(videoPkt);
                    }
                }
                else if (packet->stream_index == m_audioStreamIndex) 
                {
                    AVPacket* audioPkt = av_packet_clone(packet);
                    if (audioPkt)
                    {
                        m_audioPacketQueue.push(audioPkt);
                    }
                }
                
                av_packet_unref(packet);
            }
            else if (readResult == AVERROR_EOF)
            {
                // End of file - đánh dấu EOF, chờ queue drain trước khi báo finished
                if (m_state == PlaybackState::PLAYING) 
                {
                    m_endOfStreamReached = true;
                }
                break;
            }
            else
            {
                // Lỗi đọc frame - có thể do format không hỗ trợ
                char errBuf[AV_ERROR_MAX_STRING_SIZE] = {0};
                av_strerror(readResult, errBuf, sizeof(errBuf));
                LOG_ERROR("Demux thread: Error reading frame: " + std::string(errBuf));
                
                // Thử tiếp tục nếu không phải lỗi nghiêm trọng
                if (readResult == AVERROR_INVALIDDATA)
                {
                    av_packet_unref(packet);
                    continue;
                }
                
                // Lỗi nghiêm trọng - dừng playback
                notifyError("Format error: " + std::string(errBuf));
                break;
            }
        }
        
        av_packet_free(&packet);
    }
    catch (const std::exception& e)
    {
        LOG_ERROR("Demux thread exception: " + std::string(e.what()));
        notifyError("Demux error: " + std::string(e.what()));
    }
    catch (...)
    {
        LOG_ERROR("Demux thread: Unknown exception");
        notifyError("Unknown demux error");
    }
    
    LOG_INFO("Demux thread finished");
}

void VideoPlaybackEngine::videoDecodeThread() 
{
    LOG_INFO("Video decode thread started");
    
    try
    {
        while (!m_shouldStop) 
        {
            auto packet = m_videoPacketQueue.waitAndPop();
            
            if (!packet) 
            {
                continue;
            }
            
            // Kiểm tra codec context trước khi decode
            if (!m_videoCodecContext)
            {
                AVPacket* pkt = *packet;
                av_packet_free(&pkt);
                continue;
            }
            
            decodeVideoPacket(*packet);
            AVPacket* pkt = *packet;
            av_packet_free(&pkt);
        }
    }
    catch (const std::exception& e)
    {
        LOG_ERROR("Video decode thread exception: " + std::string(e.what()));
        notifyError("Video decode error: " + std::string(e.what()));
    }
    catch (...)
    {
        LOG_ERROR("Video decode thread: Unknown exception");
        notifyError("Unknown video decode error");
    }
    
    LOG_INFO("Video decode thread finished");
}

void VideoPlaybackEngine::audioDecodeThread() 
{
    LOG_INFO("Audio decode thread started");
    
    try
    {
        while (!m_shouldStop) 
        {
            auto packet = m_audioPacketQueue.waitAndPop();
            
            if (!packet) 
            {
                continue;
            }
            
            // Kiểm tra audio codec context trước khi decode
            if (!m_audioCodecContext)
            {
                AVPacket* pkt = *packet;
                av_packet_free(&pkt);
                continue;
            }
            
            decodeAudioPacket(*packet);
            AVPacket* pkt = *packet;
            av_packet_free(&pkt);
        }
    }
    catch (const std::exception& e)
    {
        LOG_ERROR("Audio decode thread exception: " + std::string(e.what()));
        notifyError("Audio decode error: " + std::string(e.what()));
    }
    catch (...)
    {
        LOG_ERROR("Audio decode thread: Unknown exception");
        notifyError("Unknown audio decode error");
    }
    
    LOG_INFO("Audio decode thread finished");
}

bool VideoPlaybackEngine::decodeVideoPacket(AVPacket* packet) 
{
    if (!m_videoCodecContext) 
    {
        return false;
    }

    int sendResult = avcodec_send_packet(m_videoCodecContext, packet);
    if (sendResult < 0) 
    {
        // Bỏ qua lỗi invalid data, tiếp tục với packet tiếp theo
        if (sendResult == AVERROR_INVALIDDATA)
        {
            return true;
        }
        LOG_ERROR("Error sending video packet to decoder");
        return false;
    }
    
    while (true) 
    {
        AVFrame* frame = av_frame_alloc();
        
        int ret = avcodec_receive_frame(m_videoCodecContext, frame);
        
        if (ret == AVERROR(EAGAIN) || ret == AVERROR_EOF) 
        {
            av_frame_free(&frame);
            break;
        }
        
        if (ret < 0) 
        {
            LOG_ERROR("Error receiving video frame from decoder");
            av_frame_free(&frame);
            return false;
        }
        
        // Calculate PTS
        double pts = 0.0;
        if (frame->pts != AV_NOPTS_VALUE) 
        {
            AVRational timeBase = m_formatContext->streams[m_videoStreamIndex]->time_base;
            pts = frame->pts * av_q2d(timeBase);
        }
        
        VideoFrame* videoFrame = new VideoFrame();
        videoFrame->frame = frame;
        videoFrame->pts = pts;
        
        m_videoFrameQueue.push(videoFrame);
    }
    
    return true;
}

bool VideoPlaybackEngine::decodeAudioPacket(AVPacket* packet) 
{
    // Kiểm tra null pointers
    if (!packet || !m_audioCodecContext)
    {
        return false;
    }
    
    int sendResult = avcodec_send_packet(m_audioCodecContext, packet);
    if (sendResult < 0) 
    {
        // Bỏ qua lỗi invalid data, tiếp tục với packet tiếp theo
        if (sendResult == AVERROR_INVALIDDATA)
        {
            return true;
        }
        LOG_ERROR("Error sending audio packet to decoder");
        return false;
    }
    
    while (true) 
    {
        AVFrame* frame = av_frame_alloc();
        
        int ret = avcodec_receive_frame(m_audioCodecContext, frame);
        
        if (ret == AVERROR(EAGAIN) || ret == AVERROR_EOF) 
        {
            av_frame_free(&frame);
            break;
        }
        
        if (ret < 0) 
        {
            LOG_ERROR("Error receiving audio frame from decoder");
            av_frame_free(&frame);
            return false;
        }
        
        // Calculate PTS
        double pts = 0.0;
        if (frame->pts != AV_NOPTS_VALUE) 
        {
            AVRational timeBase = m_formatContext->streams[m_audioStreamIndex]->time_base;
            pts = frame->pts * av_q2d(timeBase);
        }
        
        // Convert audio to desired format (simplified)
        int bufferSize = av_samples_get_buffer_size(nullptr, 
                                                     m_audioCodecContext->ch_layout.nb_channels, 
                                                     frame->nb_samples, 
                                                     AV_SAMPLE_FMT_S16, 
                                                     1);
        
        AudioFrame* audioFrame = new AudioFrame();
        audioFrame->data = static_cast<uint8_t*>(av_malloc(bufferSize));
        audioFrame->size = bufferSize;
        audioFrame->pts = pts;
        
        // Copy audio data (simplified - should use swr_convert)
        memcpy(audioFrame->data, frame->data[0], bufferSize);
        
        m_audioFrameQueue.push(audioFrame);
        
        av_frame_free(&frame);
    }
    
    return true;
}

void VideoPlaybackEngine::videoRenderThread() 
{
    LOG_INFO("Video render thread started");
    
    try
    {
        while (!m_shouldStop) 
        {
            if (m_state != PlaybackState::PLAYING) 
            {
                std::this_thread::sleep_for(std::chrono::milliseconds(10));
                continue;
            }
            
            auto frame = m_videoFrameQueue.pop();
            
            if (!frame) 
            {
                std::this_thread::sleep_for(std::chrono::milliseconds(1));
                continue;
            }
            
            // Kiểm tra frame hợp lệ trước khi xử lý
            if (*frame && (*frame)->frame)
            {
                synchronizeVideo(*frame);
                
                AVFrame* avFrame = (*frame)->frame;
                
                // Convert frame sang YUV420P và lưu vào buffer
                // KHÔNG gọi SDL functions từ thread này
                if (m_swsContext && avFrame)
                {
                    AVFrame* yuvFrame = av_frame_alloc();
                    if (yuvFrame)
                    {
                        yuvFrame->format = AV_PIX_FMT_YUV420P;
                        yuvFrame->width = m_videoWidth;
                        yuvFrame->height = m_videoHeight;
                        
                        if (av_frame_get_buffer(yuvFrame, 0) >= 0)
                        {
                            sws_scale(m_swsContext,
                                      avFrame->data,
                                      avFrame->linesize,
                                      0,
                                      m_videoHeight,
                                      yuvFrame->data,
                                      yuvFrame->linesize);
                            
                            // Lưu frame data vào buffer (thread-safe)
                            {
                                std::lock_guard<std::mutex> lock(m_textureMutex);
                                
                                int ySize = yuvFrame->linesize[0] * m_videoHeight;
                                int uSize = yuvFrame->linesize[1] * m_videoHeight / 2;
                                int vSize = yuvFrame->linesize[2] * m_videoHeight / 2;
                                
                                m_yPlane.assign(yuvFrame->data[0], yuvFrame->data[0] + ySize);
                                m_uPlane.assign(yuvFrame->data[1], yuvFrame->data[1] + uSize);
                                m_vPlane.assign(yuvFrame->data[2], yuvFrame->data[2] + vSize);
                                
                                m_yPitch = yuvFrame->linesize[0];
                                m_uPitch = yuvFrame->linesize[1];
                                m_vPitch = yuvFrame->linesize[2];
                                
                                m_frameReady = true;
                            }
                            
                            // Push event để main thread update texture
                            SDL_Event event;
                            SDL_zero(event);
                            event.type = core::EVENT_VIDEO_FRAME_READY;
                            SDL_PushEvent(&event);
                        }
                        
                        av_frame_free(&yuvFrame);
                    }
                }
            }
            
            delete *frame;
        }
    }
    catch (const std::exception& e)
    {
        LOG_ERROR("Video render thread exception: " + std::string(e.what()));
        notifyError("Video render error: " + std::string(e.what()));
    }
    catch (...)
    {
        LOG_ERROR("Video render thread: Unknown exception");
        notifyError("Unknown video render error");
    }
    
    LOG_INFO("Video render thread finished");
}

void VideoPlaybackEngine::renderVideoFrame(VideoFrame* frame) 
{
    // Kiểm tra null pointers trước khi render
    if (!frame || !frame->frame)
    {
        return;
    }
    
    std::lock_guard<std::mutex> lock(m_textureMutex);
    
    // Kiểm tra renderer và texture hợp lệ
    if (!m_renderer || !m_texture)
    {
        return;
    }
    
    updateVideoTexture(frame->frame);
    
    SDL_RenderClear(m_renderer);
    SDL_RenderCopy(m_renderer, m_texture, nullptr, nullptr);
    SDL_RenderPresent(m_renderer);
}

void VideoPlaybackEngine::updateVideoTexture(AVFrame* frame) 
{
    // Kiểm tra null pointers
    if (!frame || !m_swsContext || !m_texture)
    {
        return;
    }
    
    AVFrame* yuvFrame = av_frame_alloc();
    if (!yuvFrame)
    {
        LOG_ERROR("Failed to allocate YUV frame");
        return;
    }
    
    yuvFrame->format = AV_PIX_FMT_YUV420P;
    yuvFrame->width = m_videoWidth;
    yuvFrame->height = m_videoHeight;
    
    if (av_frame_get_buffer(yuvFrame, 0) < 0)
    {
        LOG_ERROR("Failed to allocate YUV frame buffer");
        av_frame_free(&yuvFrame);
        return;
    }
    
    sws_scale(m_swsContext,
              frame->data,
              frame->linesize,
              0,
              m_videoHeight,
              yuvFrame->data,
              yuvFrame->linesize);
    
    SDL_UpdateYUVTexture(m_texture,
                         nullptr,
                         yuvFrame->data[0],
                         yuvFrame->linesize[0],
                         yuvFrame->data[1],
                         yuvFrame->linesize[1],
                         yuvFrame->data[2],
                         yuvFrame->linesize[2]);
    
    av_frame_free(&yuvFrame);
}

void VideoPlaybackEngine::synchronizeVideo(VideoFrame* frame) 
{
    // Kiểm tra null pointer
    if (!frame)
    {
        return;
    }
    
    double delay = frame->pts - m_videoClock;
    
    // Giới hạn delay hợp lý để tránh treo
    if (delay > 0 && delay < 1.0) 
    {
        std::this_thread::sleep_for(
            std::chrono::duration<double>(delay)
        );
    }
    
    m_videoClock = frame->pts;
}

void VideoPlaybackEngine::audioCallback(void* userdata, Uint8* stream, int len) 
{
    // Kiểm tra null pointers
    if (!userdata || !stream || len <= 0)
    {
        if (stream && len > 0)
        {
            SDL_memset(stream, 0, len);
        }
        return;
    }
    
    VideoPlaybackEngine* engine = static_cast<VideoPlaybackEngine*>(userdata);
    engine->fillAudioBuffer(stream, len);
}

void VideoPlaybackEngine::fillAudioBuffer(Uint8* stream, int len) 
{
    std::lock_guard<std::mutex> lock(m_audioMutex);
    
    SDL_memset(stream, 0, len);
    
    // Kiểm tra trạng thái playback
    if (m_state != PlaybackState::PLAYING)
    {
        return;
    }
    
    int remaining = len;
    
    try
    {
        while (remaining > 0) 
        {
            if (m_audioBufferIndex >= m_audioBuffer.size()) 
            {
                // Get next audio frame
                auto frame = m_audioFrameQueue.pop();
                
                if (!frame) 
                {
                    break;
                }
                
                // Kiểm tra frame hợp lệ
                if (*frame && (*frame)->data && (*frame)->size > 0)
                {
                    m_audioBuffer.assign((*frame)->data, (*frame)->data + (*frame)->size);
                    m_audioBufferIndex = 0;
                    m_audioClock = (*frame)->pts;
                }
                
                delete *frame;
                
                // Nếu buffer vẫn rỗng sau khi copy, thoát
                if (m_audioBuffer.empty())
                {
                    break;
                }
            }
            
            int toCopy = std::min(remaining, static_cast<int>(m_audioBuffer.size() - m_audioBufferIndex));
            
            if (toCopy > 0)
            {
                SDL_MixAudioFormat(stream + (len - remaining),
                                  &m_audioBuffer[m_audioBufferIndex],
                                  m_audioSpec.format,
                                  toCopy,
                                  SDL_MIX_MAXVOLUME * m_volume / 100);
                
                m_audioBufferIndex += toCopy;
                remaining -= toCopy;
            }
            else
            {
                break;
            }
        }
    }
    catch (...)
    {
        // Ignore exceptions in audio callback - just output silence
    }
}

void VideoPlaybackEngine::updateThread() 
{
    while (!m_shouldStop) 
    {
        if (m_state == PlaybackState::PLAYING) 
        {
            m_currentPositionSeconds = static_cast<int>(getMasterClock());
            notifyPosition();
        }

        if (m_state == PlaybackState::PLAYING && m_endOfStreamReached && !m_finishedEventQueued && !m_shouldStop)
        {
            // Đợi decode/render/audio drain xong mới phát sự kiện finished
            if (isPlaybackDrained())
            {
                m_finishedEventQueued = true;
                notifyFinished();
            }
        }
        
        std::this_thread::sleep_for(
            std::chrono::milliseconds(config::AppConfig::PLAYBACK_UPDATE_INTERVAL_MS)
        );
    }
}

double VideoPlaybackEngine::getAudioClock() const 
{
    return m_audioClock;
}

double VideoPlaybackEngine::getVideoClock() const 
{
    return m_videoClock;
}

double VideoPlaybackEngine::getMasterClock() const 
{
    // Use audio clock as master
    if (m_audioStreamIndex != -1) 
    {
        return getAudioClock();
    }
    
    return getVideoClock();
}

void VideoPlaybackEngine::flushQueues() 
{
    // Clear video queues
    while (auto frame = m_videoFrameQueue.pop()) 
    {
        delete *frame;
    }
    
    while (auto packet = m_videoPacketQueue.pop()) 
    {
        AVPacket* pkt = *packet;
        av_packet_free(&pkt);
    }
    
    // Clear audio queues
    while (auto frame = m_audioFrameQueue.pop()) 
    {
        delete *frame;
    }
    
    while (auto packet = m_audioPacketQueue.pop()) 
    {
        AVPacket* pkt = *packet;
        av_packet_free(&pkt);
    }
    
    m_audioBuffer.clear();
    m_audioBufferIndex = 0;
    
    // Reset các queue sau khi xóa để có thể sử dụng lại
    m_videoPacketQueue.reset();
    m_audioPacketQueue.reset();
    m_videoFrameQueue.reset();
    m_audioFrameQueue.reset();
}

void VideoPlaybackEngine::notifyStateChange(PlaybackState newState) 
{
    if (m_stateChangeCallback) 
    {
        m_stateChangeCallback(newState);
    }
}

void VideoPlaybackEngine::notifyPosition() 
{
    if (m_positionCallback) 
    {
        m_positionCallback(m_currentPositionSeconds, m_totalDurationSeconds);
    }
}

void VideoPlaybackEngine::notifyError(const std::string& error) 
{
    if (m_errorCallback) 
    {
        m_errorCallback(error);
    }
}

void VideoPlaybackEngine::notifyFinished() 
{
    // Fix deadlock: Push event to main thread instead of calling callback directly
    // This allows the demux thread to finish cleanly before stop() joins it
    SDL_Event event;
    SDL_zero(event);
    event.type = SDL_USEREVENT + 1; // EVENT_PLAYBACK_FINISHED (defined in Application.h but verify value matches)
    event.user.code = 0;
    event.user.data1 = nullptr;
    event.user.data2 = nullptr;
    
    SDL_PushEvent(&event);
    
    // Legacy callback (optional, but PlaybackController relies on event loop now)
    if (m_finishedCallback) 
    {
        // m_finishedCallback(); // Disabled to prevent deadlock
    }
}

} // namespace services
} // namespace media_player