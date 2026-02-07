// Project includes
#include "services/MetadataReader.h"
#include "utils/Logger.h"

// TagLib includes
#include <taglib/fileref.h>
#include <taglib/tag.h>
#include <taglib/audioproperties.h>
#include <taglib/mpegfile.h>
#include <taglib/id3v2tag.h>
#include <taglib/id3v2frame.h>
#include <taglib/attachedpictureframe.h>

// System includes
#include <fstream>
#include <algorithm>

namespace media_player 
{
namespace services 
{

MetadataReader::MetadataReader() 
{
    LOG_INFO("MetadataReader initialized");
}

MetadataReader::~MetadataReader() 
{
    LOG_INFO("MetadataReader destroyed");
}

std::unique_ptr<models::MetadataModel> MetadataReader::readMetadata(const std::string& filePath) 
{
    if (!canReadFile(filePath)) 
    {
        LOG_ERROR("Cannot read file: " + filePath);
        return nullptr;
    }
    
    auto metadata = std::make_unique<models::MetadataModel>();
    
    try 
    {
        TagLib::FileRef file(filePath.c_str());
        
        if (file.isNull()) 
        {
            LOG_ERROR("TagLib could not open file: " + filePath);
            return nullptr;
        }
        
        TagLib::Tag* tag = file.tag();
        
        if (tag) 
        {
            metadata->setTitle(tag->title().toCString(true));
            metadata->setArtist(tag->artist().toCString(true));
            metadata->setAlbum(tag->album().toCString(true));
            metadata->setGenre(tag->genre().toCString(true));
            metadata->setYear(std::to_string(tag->year()));
        }
        
        TagLib::AudioProperties* properties = file.audioProperties();
        
        if (properties) 
        {
            // Duration in seconds
            int duration = properties->lengthInSeconds();
            metadata->setCustomTag("duration", std::to_string(duration));
            
            // Bitrate in kbps
            int bitrate = properties->bitrate();
            metadata->setCustomTag("bitrate", std::to_string(bitrate));
        }
        
        // Extract cover art for MP3 files
        std::string extension = getFileExtension(filePath);
        
        if (extension == ".mp3") 
        {
            TagLib::MPEG::File mp3File(filePath.c_str());
            
            if (mp3File.isValid() && mp3File.ID3v2Tag()) 
            {
                TagLib::ID3v2::Tag* id3v2 = mp3File.ID3v2Tag();
                TagLib::ID3v2::FrameList frameList = id3v2->frameList("APIC");
                
                if (!frameList.isEmpty()) 
                {
                    TagLib::ID3v2::AttachedPictureFrame* frame = 
                        static_cast<TagLib::ID3v2::AttachedPictureFrame*>(frameList.front());
                    
                    if (frame) 
                    {
                        TagLib::ByteVector pictureData = frame->picture();
                        std::vector<unsigned char> coverData(pictureData.begin(), pictureData.end());
                        
                        metadata->setCustomTag("cover_art_available", "true");
                        metadata->setCustomTag("cover_art_size", std::to_string(coverData.size()));
                    }
                }
            }
        }
        
        LOG_INFO("Successfully read metadata from: " + filePath);
        return metadata;
    }
    catch (const std::exception& e) 
    {
        LOG_ERROR("Exception reading metadata: " + std::string(e.what()));
        return nullptr;
    }
}

bool MetadataReader::canReadFile(const std::string& filePath) const 
{
    return isSupportedFormat(filePath);
}

bool MetadataReader::writeMetadata(const std::string& filePath, const models::MetadataModel& metadata) 
{
    if (!canReadFile(filePath)) 
    {
        LOG_ERROR("Cannot write to file: " + filePath);
        return false;
    }
    
    try 
    {
        TagLib::FileRef file(filePath.c_str());
        
        if (file.isNull()) 
        {
            LOG_ERROR("TagLib could not open file for writing: " + filePath);
            return false;
        }
        
        TagLib::Tag* tag = file.tag();
        
        if (!tag) 
        {
            LOG_ERROR("No tag available in file: " + filePath);
            return false;
        }
        
        // Update basic metadata
        tag->setTitle(metadata.getTitle());
        tag->setArtist(metadata.getArtist());
        tag->setAlbum(metadata.getAlbum());
        tag->setGenre(metadata.getGenre());
        
        // Parse year string to unsigned int
        try 
        {
            if (!metadata.getYear().empty()) 
            {
                unsigned int year = std::stoul(metadata.getYear());
                tag->setYear(year);
            }
        }
        catch (const std::exception& e) 
        {
            LOG_WARNING("Invalid year format: " + metadata.getYear());
        }
        
        bool success = file.save();
        
        if (success) 
        {
            LOG_INFO("Successfully wrote metadata to: " + filePath);
        }
        else 
        {
            LOG_ERROR("Failed to save metadata to: " + filePath);
        }
        
        return success;
    }
    catch (const std::exception& e) 
    {
        LOG_ERROR("Exception writing metadata: " + std::string(e.what()));
        return false;
    }
}

bool MetadataReader::extractCoverArt(const std::string& filePath, const std::string& outputPath) 
{
    std::string extension = getFileExtension(filePath);
    
    if (extension != ".mp3") 
    {
        LOG_WARNING("Cover art extraction only supported for MP3 files");
        return false;
    }
    
    try 
    {
        TagLib::MPEG::File mp3File(filePath.c_str());
        
        if (!mp3File.isValid() || !mp3File.ID3v2Tag()) 
        {
            LOG_ERROR("Invalid MP3 file or no ID3v2 tag: " + filePath);
            return false;
        }
        
        TagLib::ID3v2::Tag* id3v2 = mp3File.ID3v2Tag();
        TagLib::ID3v2::FrameList frameList = id3v2->frameList("APIC");
        
        if (frameList.isEmpty()) 
        {
            LOG_WARNING("No cover art found in: " + filePath);
            return false;
        }
        
        TagLib::ID3v2::AttachedPictureFrame* frame = 
            static_cast<TagLib::ID3v2::AttachedPictureFrame*>(frameList.front());
        
        if (!frame) 
        {
            return false;
        }
        
        TagLib::ByteVector pictureData = frame->picture();
        std::vector<unsigned char> coverData(pictureData.begin(), pictureData.end());
        
        bool success = writeImageFile(outputPath, coverData);
        
        if (success) 
        {
            LOG_INFO("Cover art extracted to: " + outputPath);
        }
        
        return success;
    }
    catch (const std::exception& e) 
    {
        LOG_ERROR("Exception extracting cover art: " + std::string(e.what()));
        return false;
    }
}

bool MetadataReader::embedCoverArt(const std::string& filePath, const std::string& imagePath) 
{
    std::string extension = getFileExtension(filePath);
    
    if (extension != ".mp3") 
    {
        LOG_WARNING("Cover art embedding only supported for MP3 files");
        return false;
    }
    
    try 
    {
        std::vector<unsigned char> imageData = readImageFile(imagePath);
        
        if (imageData.empty()) 
        {
            LOG_ERROR("Failed to read image file: " + imagePath);
            return false;
        }
        
        TagLib::MPEG::File mp3File(filePath.c_str());
        
        if (!mp3File.isValid()) 
        {
            LOG_ERROR("Invalid MP3 file: " + filePath);
            return false;
        }
        
        TagLib::ID3v2::Tag* id3v2 = mp3File.ID3v2Tag(true);
        
        if (!id3v2) 
        {
            LOG_ERROR("Could not create/access ID3v2 tag");
            return false;
        }
        
        // Remove existing cover art
        id3v2->removeFrames("APIC");
        
        // Create new cover art frame
        TagLib::ID3v2::AttachedPictureFrame* frame = new TagLib::ID3v2::AttachedPictureFrame();
        frame->setMimeType("image/jpeg");
        frame->setPicture(TagLib::ByteVector(reinterpret_cast<const char*>(imageData.data()), 
                                              imageData.size()));
        frame->setType(TagLib::ID3v2::AttachedPictureFrame::FrontCover);
        
        id3v2->addFrame(frame);
        
        bool success = mp3File.save();
        
        if (success) 
        {
            LOG_INFO("Cover art embedded successfully in: " + filePath);
        }
        else 
        {
            LOG_ERROR("Failed to save file with cover art: " + filePath);
        }
        
        return success;
    }
    catch (const std::exception& e) 
    {
        LOG_ERROR("Exception embedding cover art: " + std::string(e.what()));
        return false;
    }
}

bool MetadataReader::isSupportedFormat(const std::string& filePath) const 
{
    std::string extension = getFileExtension(filePath);
    
    // Chỉ hỗ trợ các định dạng ổn định: audio (.mp3, .wav) và video (.avi, .mp4)
    std::vector<std::string> supportedFormats = {".mp3", ".wav", ".avi", ".mp4"};
    
    return std::find(supportedFormats.begin(), 
                     supportedFormats.end(), 
                     extension) != supportedFormats.end();
}

std::string MetadataReader::getFileExtension(const std::string& filePath) const 
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

std::vector<unsigned char> MetadataReader::readImageFile(const std::string& imagePath) const 
{
    std::ifstream file(imagePath, std::ios::binary | std::ios::ate);
    
    if (!file.is_open()) 
    {
        return {};
    }
    
    std::streamsize size = file.tellg();
    file.seekg(0, std::ios::beg);
    
    std::vector<unsigned char> buffer(size);
    
    if (!file.read(reinterpret_cast<char*>(buffer.data()), size)) 
    {
        return {};
    }
    
    return buffer;
}

bool MetadataReader::writeImageFile(const std::string& outputPath, 
                                    const std::vector<unsigned char>& data) const 
{
    std::ofstream file(outputPath, std::ios::binary);
    
    if (!file.is_open()) 
    {
        return false;
    }
    
    file.write(reinterpret_cast<const char*>(data.data()), data.size());
    
    return file.good();
}

} // namespace services
} // namespace media_player