// Project includes
#include "services/VideoMetadataReader.h"
#include "utils/Logger.h"
#include "config/AppConfig.h"

// FFmpeg includes
extern "C" 
{
#include <libavformat/avformat.h>
#include <libavcodec/avcodec.h>
#include <libavutil/imgutils.h>
#include <libswscale/swscale.h>
}

// System includes
#include <algorithm>

namespace media_player 
{
namespace services 
{

VideoMetadataReader::VideoMetadataReader() 
{
    LOG_INFO("VideoMetadataReader initialized");
}

VideoMetadataReader::~VideoMetadataReader() 
{
    LOG_INFO("VideoMetadataReader destroyed");
}

std::unique_ptr<models::VideoMetadataModel> VideoMetadataReader::readMetadata(const std::string& filePath) 
{
    if (!canReadFile(filePath)) 
    {
        LOG_ERROR("Cannot read video file: " + filePath);
        return nullptr;
    }
    
    auto metadata = std::make_unique<models::VideoMetadataModel>();
    
    AVFormatContext* formatContext = nullptr;
    
    if (avformat_open_input(&formatContext, filePath.c_str(), nullptr, nullptr) != 0) 
    {
        LOG_ERROR("Could not open video file: " + filePath);
        return nullptr;
    }
    
    if (avformat_find_stream_info(formatContext, nullptr) < 0) 
    {
        LOG_ERROR("Could not find stream info: " + filePath);
        avformat_close_input(&formatContext);
        return nullptr;
    }
    
    // Find video and audio streams
    int videoStreamIndex = -1;
    int audioStreamIndex = -1;
    
    for (unsigned int i = 0; i < formatContext->nb_streams; i++) 
    {
        AVCodecParameters* codecParams = formatContext->streams[i]->codecpar;
        
        if (codecParams->codec_type == AVMEDIA_TYPE_VIDEO && videoStreamIndex == -1) 
        {
            videoStreamIndex = i;
        }
        else if (codecParams->codec_type == AVMEDIA_TYPE_AUDIO && audioStreamIndex == -1) 
        {
            audioStreamIndex = i;
        }
    }
    
    if (videoStreamIndex == -1) 
    {
        LOG_ERROR("No video stream found in file: " + filePath);
        avformat_close_input(&formatContext);
        return nullptr;
    }
    
    // Extract video metadata
    AVStream* videoStream = formatContext->streams[videoStreamIndex];
    AVCodecParameters* videoCodecParams = videoStream->codecpar;
    
    metadata->setResolution(videoCodecParams->width, videoCodecParams->height);
    
    // Frame rate
    AVRational frameRate = videoStream->avg_frame_rate;
    if (frameRate.den != 0) 
    {
        double fps = static_cast<double>(frameRate.num) / static_cast<double>(frameRate.den);
        metadata->setFrameRate(fps);
    }
    
    // Duration
    if (formatContext->duration != AV_NOPTS_VALUE) 
    {
        int durationSeconds = formatContext->duration / AV_TIME_BASE;
        metadata->setDuration(durationSeconds);
    }
    
    // Bitrate
    if (formatContext->bit_rate > 0) 
    {
        metadata->setBitrate(formatContext->bit_rate);
    }
    
    // Video codec
    const AVCodec* videoCodec = avcodec_find_decoder(videoCodecParams->codec_id);
    if (videoCodec) 
    {
        metadata->setVideoCodec(videoCodec->name);
    }
    
    // Extract audio metadata if available
    if (audioStreamIndex != -1) 
    {
        AVCodecParameters* audioCodecParams = formatContext->streams[audioStreamIndex]->codecpar;
        
        const AVCodec* audioCodec = avcodec_find_decoder(audioCodecParams->codec_id);
        if (audioCodec) 
        {
            metadata->setAudioCodec(audioCodec->name);
        }
        
        metadata->setAudioChannels(audioCodecParams->channels);
        metadata->setAudioSampleRate(audioCodecParams->sample_rate);
    }
    
    // Title from metadata
    AVDictionaryEntry* titleEntry = av_dict_get(formatContext->metadata, "title", nullptr, 0);
    if (titleEntry) 
    {
        metadata->setTitle(titleEntry->value);
    }
    
    avformat_close_input(&formatContext);
    
    LOG_INFO("Successfully read video metadata from: " + filePath);
    return metadata;
}

bool VideoMetadataReader::canReadFile(const std::string& filePath) const 
{
    return isSupportedFormat(filePath);
}

bool VideoMetadataReader::extractThumbnail(const std::string& filePath, 
                                          const std::string& outputPath, 
                                          int timeSeconds) 
{
    AVFormatContext* formatContext = nullptr;
    
    if (avformat_open_input(&formatContext, filePath.c_str(), nullptr, nullptr) != 0) 
    {
        LOG_ERROR("Could not open video file for thumbnail: " + filePath);
        return false;
    }
    
    if (avformat_find_stream_info(formatContext, nullptr) < 0) 
    {
        avformat_close_input(&formatContext);
        return false;
    }
    
    int videoStreamIndex = -1;
    for (unsigned int i = 0; i < formatContext->nb_streams; i++) 
    {
        if (formatContext->streams[i]->codecpar->codec_type == AVMEDIA_TYPE_VIDEO) 
        {
            videoStreamIndex = i;
            break;
        }
    }
    
    if (videoStreamIndex == -1) 
    {
        avformat_close_input(&formatContext);
        return false;
    }
    
    AVStream* videoStream = formatContext->streams[videoStreamIndex];
    AVCodecParameters* codecParams = videoStream->codecpar;
    
    const AVCodec* codec = avcodec_find_decoder(codecParams->codec_id);
    if (!codec) 
    {
        avformat_close_input(&formatContext);
        return false;
    }
    
    AVCodecContext* codecContext = avcodec_alloc_context3(codec);
    avcodec_parameters_to_context(codecContext, codecParams);
    
    if (avcodec_open2(codecContext, codec, nullptr) < 0) 
    {
        avcodec_free_context(&codecContext);
        avformat_close_input(&formatContext);
        return false;
    }
    
    // Seek to desired time
    int64_t timestamp = timeSeconds * AV_TIME_BASE;
    av_seek_frame(formatContext, -1, timestamp, AVSEEK_FLAG_BACKWARD);
    
    AVPacket* packet = av_packet_alloc();
    AVFrame* frame = av_frame_alloc();
    
    bool thumbnailExtracted = false;
    
    while (av_read_frame(formatContext, packet) >= 0) 
    {
        if (packet->stream_index == videoStreamIndex) 
        {
            if (avcodec_send_packet(codecContext, packet) == 0) 
            {
                if (avcodec_receive_frame(codecContext, frame) == 0) 
                {
                    // Frame extracted - save as image (simplified)
                    // In real implementation, use libpng or libjpeg to save
                    LOG_INFO("Thumbnail frame extracted (saving not implemented)");
                    thumbnailExtracted = true;
                    break;
                }
            }
        }
        av_packet_unref(packet);
    }
    
    av_frame_free(&frame);
    av_packet_free(&packet);
    avcodec_free_context(&codecContext);
    avformat_close_input(&formatContext);
    
    return thumbnailExtracted;
}

bool VideoMetadataReader::isSupportedFormat(const std::string& filePath) const 
{
    std::string extension = getFileExtension(filePath);
    const auto& videoExts = config::AppConfig::VIDEO_EXTENSIONS;
    
    return std::find(videoExts.begin(), videoExts.end(), extension) != videoExts.end();
}

std::string VideoMetadataReader::getFileExtension(const std::string& filePath) const 
{
    size_t dotPos = filePath.find_last_of('.');
    
    if (dotPos == std::string::npos) 
    {
        return "";
    }
    
    std::string extension = filePath.substr(dotPos);
    std::transform(extension.begin(), extension.end(), extension.begin(), ::tolower);
    
    return extension;
}

} // namespace services
} // namespace media_player