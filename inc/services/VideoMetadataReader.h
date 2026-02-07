#ifndef VIDEO_METADATA_READER_H
#define VIDEO_METADATA_READER_H

// System includes
#include <string>
#include <memory>

// Project includes
#include "IVideoMetadataReader.h"
#include "models/VideoMetadataModel.h"

namespace media_player 
{
namespace services 
{

class VideoMetadataReader : public IVideoMetadataReader 
{
public:
    VideoMetadataReader();
    ~VideoMetadataReader();
    
    // IVideoMetadataReader interface implementation
    std::unique_ptr<models::VideoMetadataModel> readMetadata(const std::string& filePath) override;
    bool canReadFile(const std::string& filePath) const override;
    
    bool extractThumbnail(const std::string& filePath, 
                         const std::string& outputPath, 
                         int timeSeconds = 0) override;
    
private:
    bool isSupportedFormat(const std::string& filePath) const;
    std::string getFileExtension(const std::string& filePath) const;
};

} // namespace services
} // namespace media_player

#endif // VIDEO_METADATA_READER_H