#ifndef VIDEO_METADATA_MODEL_H
#define VIDEO_METADATA_MODEL_H

// System includes
#include <string>
#include <optional>

namespace media_player 
{
namespace models 
{

class VideoMetadataModel 
{
public:
    VideoMetadataModel() = default;
    explicit VideoMetadataModel(const std::string& filePath);
    
    // Video properties getters
    std::string getTitle() const 
    { 
        return m_title; 
    }
    
    int getWidth() const 
    { 
        return m_width; 
    }
    
    int getHeight() const 
    { 
        return m_height; 
    }
    
    double getFrameRate() const 
    { 
        return m_frameRate; 
    }
    
    int getDuration() const 
    { 
        return m_durationSeconds; 
    }
    
    long long getBitrate() const 
    { 
        return m_bitrate; 
    }
    
    std::string getVideoCodec() const 
    { 
        return m_videoCodec; 
    }
    
    std::string getAudioCodec() const 
    { 
        return m_audioCodec; 
    }
    
    int getAudioChannels() const 
    { 
        return m_audioChannels; 
    }
    
    int getAudioSampleRate() const 
    { 
        return m_audioSampleRate; 
    }
    
    // Setters
    void setTitle(const std::string& title) 
    { 
        m_title = title; 
    }
    
    void setResolution(int width, int height) 
    {
        m_width = width;
        m_height = height;
    }
    
    void setFrameRate(double fps) 
    { 
        m_frameRate = fps; 
    }
    
    void setDuration(int seconds) 
    { 
        m_durationSeconds = seconds; 
    }
    
    void setBitrate(long long bitrate) 
    { 
        m_bitrate = bitrate; 
    }
    
    void setVideoCodec(const std::string& codec) 
    { 
        m_videoCodec = codec; 
    }
    
    void setAudioCodec(const std::string& codec) 
    { 
        m_audioCodec = codec; 
    }
    
    void setAudioChannels(int channels) 
    { 
        m_audioChannels = channels; 
    }
    
    void setAudioSampleRate(int sampleRate) 
    { 
        m_audioSampleRate = sampleRate; 
    }
    
    // Helpers
    std::string getResolutionString() const;
    std::string getFormattedDuration() const;
    std::string getFormattedBitrate() const;
    double getAspectRatio() const;
    
    bool hasAudioTrack() const 
    { 
        return !m_audioCodec.empty(); 
    }
    
    bool isValid() const;
    
private:
    std::string m_title;
    int m_width = 0;
    int m_height = 0;
    double m_frameRate = 0.0;
    int m_durationSeconds = 0;
    long long m_bitrate = 0;
    
    std::string m_videoCodec;
    std::string m_audioCodec;
    int m_audioChannels = 0;
    int m_audioSampleRate = 0;
};

} // namespace models
} // namespace media_player

#endif // VIDEO_METADATA_MODEL_H