#ifndef METADATA_READER_H
#define METADATA_READER_H

// System includes
#include <string>
#include <memory>

// Project includes
#include "IMetadataReader.h"
#include "models/MetadataModel.h"

namespace media_player 
{
namespace services 
{

class MetadataReader : public IMetadataReader 
{
public:
    MetadataReader();
    ~MetadataReader();
    
    // IMetadataReader interface implementation
    std::unique_ptr<models::MetadataModel> readMetadata(const std::string& filePath) override;
    bool canReadFile(const std::string& filePath) const override;
    
    bool writeMetadata(const std::string& filePath, const models::MetadataModel& metadata) override;
    
    bool extractCoverArt(const std::string& filePath, const std::string& outputPath) override;
    bool embedCoverArt(const std::string& filePath, const std::string& imagePath) override;
    
private:
    bool isSupportedFormat(const std::string& filePath) const;
    std::string getFileExtension(const std::string& filePath) const;
    
    std::vector<unsigned char> readImageFile(const std::string& imagePath) const;
    bool writeImageFile(const std::string& outputPath, const std::vector<unsigned char>& data) const;
};

} // namespace services
} // namespace media_player

#endif // METADATA_READER_H