#ifndef I_VIDEO_METADATA_READER_H
#define I_VIDEO_METADATA_READER_H

// System includes
#include <string>
#include <memory>

// Project includes
#include "models/VideoMetadataModel.h"

namespace media_player 
{
namespace services 
{

class IVideoMetadataReader 
{
public:
    virtual ~IVideoMetadataReader() = default;
    
    // Read operations
    virtual std::unique_ptr<models::VideoMetadataModel> readMetadata(const std::string& filePath) = 0;
    virtual bool canReadFile(const std::string& filePath) const = 0;
    
    // Video info extraction
    virtual bool extractThumbnail(const std::string& filePath, 
                                 const std::string& outputPath, 
                                 int timeSeconds = 0) = 0;
};

} // namespace services
} // namespace media_player

#endif // I_VIDEO_METADATA_READER_H