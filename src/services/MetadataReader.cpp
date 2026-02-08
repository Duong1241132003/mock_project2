// Project includes
#include "services/MetadataReader.h"

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
}

MetadataReader::~MetadataReader() 
{
}

std::unique_ptr<models::MetadataModel> MetadataReader::readMetadata(const std::string& filePath) 
{
    if (!canReadFile(filePath)) 
    {
        return nullptr;
    }
    
    auto metadata = std::make_unique<models::MetadataModel>();
    
    try 
    {
        TagLib::FileRef file(filePath.c_str());
        
        if (file.isNull()) 
        {
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
        
        return metadata;
    }
    catch (const std::exception& e) 
    {
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
        return false;
    }
    
    try 
    {
        TagLib::FileRef file(filePath.c_str());
        
        if (file.isNull()) 
        {
            return false;
        }
        
        TagLib::Tag* tag = file.tag();
        
        if (!tag) 
        {
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
        }
        
        bool success = file.save();
        
        if (success) 
        {
        }
        else 
        {
        }
        
        return success;
    }
    catch (const std::exception& e) 
    {
        return false;
    }
}

bool MetadataReader::extractCoverArt(const std::string& filePath, const std::string& outputPath) 
{
    std::string extension = getFileExtension(filePath);
    
    if (extension != ".mp3") 
    {
        return false;
    }
    
    try 
    {
        TagLib::MPEG::File mp3File(filePath.c_str());
        
        if (!mp3File.isValid() || !mp3File.ID3v2Tag()) 
        {
            return false;
        }
        
        TagLib::ID3v2::Tag* id3v2 = mp3File.ID3v2Tag();
        TagLib::ID3v2::FrameList frameList = id3v2->frameList("APIC");
        
        if (frameList.isEmpty()) 
        {
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
        }
        
        return success;
    }
    catch (const std::exception& e) 
    {
        return false;
    }
}

bool MetadataReader::embedCoverArt(const std::string& filePath, const std::string& imagePath) 
{
    std::string extension = getFileExtension(filePath);
    
    if (extension != ".mp3") 
    {
        return false;
    }
    
    try 
    {
        std::vector<unsigned char> imageData = readImageFile(imagePath);
        
        if (imageData.empty()) 
        {
            return false;
        }
        
        TagLib::MPEG::File mp3File(filePath.c_str());
        
        if (!mp3File.isValid()) 
        {
            return false;
        }
        
        TagLib::ID3v2::Tag* id3v2 = mp3File.ID3v2Tag(true);
        
        if (!id3v2) 
        {
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
        }
        else 
        {
        }
        
        return success;
    }
    catch (const std::exception& e) 
    {
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
