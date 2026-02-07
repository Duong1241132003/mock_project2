#ifndef I_METADATA_READER_H
#define I_METADATA_READER_H

// System includes
#include <string>
#include <memory>

// Project includes
#include "models/MetadataModel.h"

namespace media_player 
{
namespace services 
{

class IMetadataReader 
{
public:
    virtual ~IMetadataReader() = default;
    
    // Read operations
    virtual std::unique_ptr<models::MetadataModel> readMetadata(const std::string& filePath) = 0;
    virtual bool canReadFile(const std::string& filePath) const = 0;
    
    // Write operations
    virtual bool writeMetadata(const std::string& filePath, const models::MetadataModel& metadata) = 0;
    
    // Cover art
    virtual bool extractCoverArt(const std::string& filePath, const std::string& outputPath) = 0;
    virtual bool embedCoverArt(const std::string& filePath, const std::string& imagePath) = 0;
};

} // namespace services
} // namespace media_player

#endif // I_METADATA_READER_H