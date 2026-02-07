#ifndef MEDIA_FILE_MODEL_H
#define MEDIA_FILE_MODEL_H

// System includes
#include <string>
#include <chrono>
#include <memory>
#include <filesystem>

namespace media_player 
{
namespace models 
{

enum class MediaType 
{
    AUDIO,
    VIDEO,
    UNKNOWN
};

class MediaFileModel 
{
public:
    // Constructors
    MediaFileModel();
    explicit MediaFileModel(const std::string& filePath);
    
    // Getters
    std::string getFilePath() const 
    { 
        return m_filePath; 
    }
    
    std::string getFileName() const 
    { 
        return m_fileName; 
    }
    
    std::string getExtension() const 
    { 
        return m_extension; 
    }
    
    MediaType getType() const 
    { 
        return m_type; 
    }
    
    size_t getFileSize() const 
    { 
        return m_fileSize; 
    }
    
    // Metadata getters
    std::string getTitle() const { return m_title; }
    std::string getArtist() const { return m_artist; }
    std::string getAlbum() const { return m_album; }
    int getDuration() const { return m_duration; }
    
    // Metadata setters
    void setTitle(const std::string& title) { m_title = title; }
    void setArtist(const std::string& artist) { m_artist = artist; }
    void setAlbum(const std::string& album) { m_album = album; }
    void setDuration(int duration) { m_duration = duration; }
    
    // Validation
    bool isValid() const;
    
    bool isAudio() const 
    { 
        return m_type == MediaType::AUDIO; 
    }
    
    bool isVideo() const 
    { 
        return m_type == MediaType::VIDEO; 
    }
    
    // Comparison operators for sorting
    bool operator<(const MediaFileModel& other) const;
    bool operator==(const MediaFileModel& other) const;
    
    // Serialization
    std::string serialize() const;
    static MediaFileModel deserialize(const std::string& data);
    
private:
    void extractFileInfo();
    MediaType determineMediaType() const;
    
    std::string m_filePath;
    std::string m_fileName;
    std::string m_extension;
    MediaType m_type;
    size_t m_fileSize;
    std::filesystem::file_time_type m_lastModified;
    
    // Metadata fields
    std::string m_title;
    std::string m_artist;
    std::string m_album;
    int m_duration = 0;
};

} // namespace models
} // namespace media_player

#endif // MEDIA_FILE_MODEL_H