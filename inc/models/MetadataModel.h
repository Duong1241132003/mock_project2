#ifndef METADATA_MODEL_H
#define METADATA_MODEL_H

// System includes
#include <string>
#include <map>
#include <optional>
#include <vector>

namespace media_player 
{
namespace models 
{

class MetadataModel 
{
public:
    MetadataModel() = default;
    explicit MetadataModel(const std::string& filePath);
    
    // Audio metadata getters
    std::string getTitle() const 
    { 
        return m_title; 
    }
    
    std::string getArtist() const 
    { 
        return m_artist; 
    }
    
    std::string getAlbum() const 
    { 
        return m_album; 
    }
    
    std::string getGenre() const 
    { 
        return m_genre; 
    }
    
    std::string getYear() const 
    { 
        return m_year; 
    }
    
    std::string getPublisher() const 
    { 
        return m_publisher; 
    }
    
    int getDuration() const 
    { 
        return m_durationSeconds; 
    }
    
    int getBitrate() const 
    { 
        return m_bitrate; 
    }
    
    // Cover art
    bool hasCoverArt() const 
    { 
        return m_hasCoverArt; 
    }
    
    std::vector<unsigned char> getCoverArt() const 
    { 
        return m_coverArtData; 
    }
    
    // Setters
    void setTitle(const std::string& title) 
    { 
        m_title = title; 
    }
    
    void setArtist(const std::string& artist) 
    { 
        m_artist = artist; 
    }
    
    void setAlbum(const std::string& album) 
    { 
        m_album = album; 
    }
    
    void setGenre(const std::string& genre) 
    { 
        m_genre = genre; 
    }
    
    void setYear(const std::string& year) 
    { 
        m_year = year; 
    }
    
    // Custom tags
    void setCustomTag(const std::string& key, const std::string& value);
    std::optional<std::string> getCustomTag(const std::string& key) const;
    
    // Validation
    bool isComplete() const;
    
    // Display
    std::string getDisplayTitle() const;
    std::string getDisplayArtist() const;
    std::string getFormattedDuration() const;
    
private:
    std::string m_title;
    std::string m_artist;
    std::string m_album;
    std::string m_genre;
    std::string m_year;
    std::string m_publisher;
    int m_durationSeconds = 0;
    int m_bitrate = 0;
    
    bool m_hasCoverArt = false;
    std::vector<unsigned char> m_coverArtData;
    
    std::map<std::string, std::string> m_customTags;
};

} // namespace models
} // namespace media_player

#endif // METADATA_MODEL_H