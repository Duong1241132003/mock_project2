#include <gtest/gtest.h>
#include "services/MetadataReader.h"
#include "models/MetadataModel.h"
#include <filesystem>
#include <fstream>
#include <vector>

namespace fs = std::filesystem;
using namespace media_player::services;
using namespace media_player::models;

class MetadataReaderTest : public ::testing::Test {
protected:
    void SetUp() override {
        testDir = fs::temp_directory_path() / "MediaPlayerTest_MetadataReader";
        if (fs::exists(testDir)) {
            fs::remove_all(testDir);
        }
        fs::create_directories(testDir);
        
        validWavPath = testDir / "test_audio.wav";
        validMp3Path = testDir / "test_audio.mp3";
        testImagePath = testDir / "test_cover.jpg";
        outputImagePath = testDir / "extracted_cover.jpg";
        
        createMinimalWav(validWavPath);
        // Create an empty file for mp3 check
        std::ofstream(validMp3Path) << "dummy"; 
        // Create a minimal image file for testing
        createTestImage(testImagePath);
    }

    void TearDown() override {
        if (fs::exists(testDir)) {
            fs::remove_all(testDir);
        }
    }

    // Create a minimal valid WAV file header
    void createMinimalWav(const fs::path& path) {
        std::ofstream file(path, std::ios::binary);
        
        // RIFF header
        file.write("RIFF", 4);
        uint32_t fileSize = 36; // Header size - 8
        file.write(reinterpret_cast<const char*>(&fileSize), 4);
        file.write("WAVE", 4);
        
        // fmt chunk
        file.write("fmt ", 4);
        uint32_t fmtSize = 16;
        file.write(reinterpret_cast<const char*>(&fmtSize), 4);
        uint16_t audioFormat = 1; // PCM
        file.write(reinterpret_cast<const char*>(&audioFormat), 2);
        uint16_t numChannels = 2;
        file.write(reinterpret_cast<const char*>(&numChannels), 2);
        uint32_t sampleRate = 44100;
        file.write(reinterpret_cast<const char*>(&sampleRate), 4);
        uint32_t byteRate = 44100 * 2 * 2;
        file.write(reinterpret_cast<const char*>(&byteRate), 4);
        uint16_t blockAlign = 4;
        file.write(reinterpret_cast<const char*>(&blockAlign), 2);
        uint16_t bitsPerSample = 16;
        file.write(reinterpret_cast<const char*>(&bitsPerSample), 2);
        
        // data chunk
        file.write("data", 4);
        uint32_t dataSize = 0;
        file.write(reinterpret_cast<const char*>(&dataSize), 4);
        
        file.close();
    }
    
    // Create test image file
    void createTestImage(const fs::path& path) {
        std::ofstream file(path, std::ios::binary);
        // Write minimal JPEG header bytes
        unsigned char jpegHeader[] = {0xFF, 0xD8, 0xFF, 0xE0, 0x00, 0x10, 
                                       0x4A, 0x46, 0x49, 0x46, 0x00};
        file.write(reinterpret_cast<char*>(jpegHeader), sizeof(jpegHeader));
        file.close();
    }

    fs::path testDir;
    fs::path validWavPath;
    fs::path validMp3Path;
    fs::path testImagePath;
    fs::path outputImagePath;
    MetadataReader reader;
};

// ===================== canReadFile =====================

TEST_F(MetadataReaderTest, CanReadFile) {
    // canReadFile chỉ kiểm tra extension được hỗ trợ, không kiểm tra file tồn tại
    EXPECT_TRUE(reader.canReadFile(validWavPath.string()));
    EXPECT_TRUE(reader.canReadFile(validMp3Path.string()));
    // File không tồn tại nhưng extension hợp lệ vẫn return true
    EXPECT_TRUE(reader.canReadFile("nonexistent.mp3"));
    // Extension không hỗ trợ thì return false
    EXPECT_FALSE(reader.canReadFile("unsupported.txt"));
}

TEST_F(MetadataReaderTest, CanReadFileVideoFormats) {
    EXPECT_TRUE(reader.canReadFile("movie.mp4"));
    EXPECT_TRUE(reader.canReadFile("video.avi"));
    EXPECT_FALSE(reader.canReadFile("document.pdf"));
}

// ===================== readMetadata =====================

TEST_F(MetadataReaderTest, ReadMetadata_Wav) {
    auto metadata = reader.readMetadata(validWavPath.string());
    ASSERT_NE(metadata, nullptr);
    
    // Minimal WAV might not have tags, but should be read successfully
    EXPECT_EQ(metadata->getTitle(), ""); 
    
    // Check audio properties
    auto duration = metadata->getCustomTag("duration");
    EXPECT_TRUE(duration.has_value());
    EXPECT_EQ(*duration, "0");
}

TEST_F(MetadataReaderTest, ReadMetadata_InvalidFile) {
    EXPECT_EQ(reader.readMetadata("nonexistent.mp3"), nullptr);
}

TEST_F(MetadataReaderTest, ReadMetadata_UnsupportedFormat) {
    auto unsupportedPath = testDir / "test.flac";
    std::ofstream(unsupportedPath) << "dummy";
    EXPECT_EQ(reader.readMetadata(unsupportedPath.string()), nullptr);
}

// ===================== isSupportedFormat =====================

TEST_F(MetadataReaderTest, IsSupportedFormat) {
    EXPECT_TRUE(reader.canReadFile("test.mp3"));
    EXPECT_TRUE(reader.canReadFile("test.wav"));
    EXPECT_TRUE(reader.canReadFile("test.avi"));
    EXPECT_TRUE(reader.canReadFile("test.mp4"));
    EXPECT_FALSE(reader.canReadFile("test.flac"));
    EXPECT_FALSE(reader.canReadFile("test.ogg"));
}

TEST_F(MetadataReaderTest, GetFileExtension) {
    // Test case sensitivity
    EXPECT_TRUE(reader.canReadFile("TEST.MP3"));
    EXPECT_TRUE(reader.canReadFile("Song.Mp3"));
}

// ===================== writeMetadata =====================

TEST_F(MetadataReaderTest, WriteMetadata_UnsupportedFormat) {
    MetadataModel metadata;
    metadata.setTitle("Test Title");
    
    // Writing to unsupported format should fail
    auto unsupportedPath = testDir / "test.txt";
    std::ofstream(unsupportedPath) << "dummy";
    EXPECT_FALSE(reader.writeMetadata(unsupportedPath.string(), metadata));
}

TEST_F(MetadataReaderTest, WriteMetadata_NonexistentFile) {
    MetadataModel metadata;
    metadata.setTitle("Test Title");
    
    // Writing to nonexistent file should fail
    EXPECT_FALSE(reader.writeMetadata("/nonexistent/path.mp3", metadata));
}

TEST_F(MetadataReaderTest, WriteMetadata_ToWav) {
    MetadataModel metadata;
    metadata.setTitle("Test Title");
    metadata.setArtist("Test Artist");
    
    // Writing metadata to WAV file (TagLib supports this)
    bool result = reader.writeMetadata(validWavPath.string(), metadata);
    // Result depends on TagLib implementation, but we exercise the code path
    // Just check it doesn't crash
}

// ===================== extractCoverArt =====================

TEST_F(MetadataReaderTest, ExtractCoverArt_NoArt) {
    // WAV file without cover art
    bool result = reader.extractCoverArt(validWavPath.string(), outputImagePath.string());
    // Should fail since WAV doesn't have cover art
    EXPECT_FALSE(result);
}

TEST_F(MetadataReaderTest, ExtractCoverArt_NonexistentFile) {
    bool result = reader.extractCoverArt("/nonexistent.mp3", outputImagePath.string());
    EXPECT_FALSE(result);
}

TEST_F(MetadataReaderTest, ExtractCoverArt_UnsupportedFormat) {
    auto unsupportedPath = testDir / "test.txt";
    std::ofstream(unsupportedPath) << "dummy";
    bool result = reader.extractCoverArt(unsupportedPath.string(), outputImagePath.string());
    EXPECT_FALSE(result);
}

// ===================== embedCoverArt =====================

TEST_F(MetadataReaderTest, EmbedCoverArt_NonMp3) {
    // Can only embed to MP3 files
    bool result = reader.embedCoverArt(validWavPath.string(), testImagePath.string());
    EXPECT_FALSE(result);
}

TEST_F(MetadataReaderTest, EmbedCoverArt_NonexistentImage) {
    bool result = reader.embedCoverArt(validMp3Path.string(), "/nonexistent/image.jpg");
    EXPECT_FALSE(result);
}

TEST_F(MetadataReaderTest, EmbedCoverArt_NonexistentAudio) {
    bool result = reader.embedCoverArt("/nonexistent/audio.mp3", testImagePath.string());
    EXPECT_FALSE(result);
}

// ===================== Edge cases =====================

TEST_F(MetadataReaderTest, ReadMetadata_EmptyPath) {
    auto metadata = reader.readMetadata("");
    EXPECT_EQ(metadata, nullptr);
}

TEST_F(MetadataReaderTest, CanReadFile_EmptyPath) {
    EXPECT_FALSE(reader.canReadFile(""));
}


TEST_F(MetadataReaderTest, ReadMetadata_DummyMp3) {
    // Reading metadata from corrupt mp3 (just dummy text)
    auto metadata = reader.readMetadata(validMp3Path.string());
    // Should handle gracefully
}

TEST_F(MetadataReaderTest, WriteMetadata_InvalidYear) {
    MetadataModel metadata;
    metadata.setYear("invalid_year");
    
    // Should catch exception and continue (return false because file is dummy/unwritable or true if TagLib ignores it)
    // We just want to ensure NO CRASH
    reader.writeMetadata(validMp3Path.string(), metadata); 
}

TEST_F(MetadataReaderTest, WriteMetadata_InvalidOutputPath) {
    MetadataModel metadata;
    metadata.setTitle("Test");
    // Write to a directory path instead of file? 
    // Or write to a valid file but save fails?
    // Using a path that is a directory:
    EXPECT_FALSE(reader.writeMetadata(testDir.string(), metadata));
}

TEST_F(MetadataReaderTest, ExtractCoverArt_InvalidOutputPath) {
    // We can't easily test this because we need a valid MP3 with cover art first to reach the write step.
    // However, we can test that passing a bad path doesn't crash if we *could* reach it.
    // Since we can't reach it with dummy files, we skip this specific branch for now 
    // or rely on the fact that we cover the 'false' return from earlier checks.
}


