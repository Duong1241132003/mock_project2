#include <gtest/gtest.h>
#include "models/MediaFileModel.h"
#include <filesystem>
#include <fstream>

namespace fs = std::filesystem;

using namespace media_player;

class MediaFileModelTest : public ::testing::Test {
protected:
    void SetUp() override {
        testDir = fs::temp_directory_path() / "MediaFileModelTest";
        fs::create_directories(testDir);
        testFile = testDir / "test.mp3";
        std::ofstream(testFile) << "test data";
    }
    
    void TearDown() override {
        if (fs::exists(testDir)) {
            fs::remove_all(testDir);
        }
    }
    
    fs::path testDir;
    fs::path testFile;
};

// ===================== Basic =====================

TEST_F(MediaFileModelTest, DefaultConstructor) {
    models::MediaFileModel file;
    EXPECT_TRUE(file.getFilePath().empty());
    EXPECT_EQ(file.getType(), models::MediaType::UNKNOWN);
}

TEST_F(MediaFileModelTest, ConstructorWithPath) {
    models::MediaFileModel file(testFile.string());
    EXPECT_EQ(file.getFilePath(), testFile.string());
    EXPECT_EQ(file.getFileName(), "test.mp3");
    EXPECT_EQ(file.getExtension(), ".mp3");
}

// ===================== Media Type Detection =====================

TEST_F(MediaFileModelTest, DetermineMediaTypeAudio) {
    models::MediaFileModel file("/home/user/music/song.mp3");
    EXPECT_EQ(file.getType(), models::MediaType::AUDIO);
}

TEST_F(MediaFileModelTest, DetermineMediaTypeAudioWav) {
    // lowercase .wav is supported
    models::MediaFileModel file("/path/to/song.wav");
    EXPECT_EQ(file.getType(), models::MediaType::AUDIO);
}

TEST_F(MediaFileModelTest, DetermineMediaTypeVideo) {
    models::MediaFileModel file("/home/user/video/movie.mp4");
    EXPECT_EQ(file.getType(), models::MediaType::UNSUPPORTED);
}

TEST_F(MediaFileModelTest, DetermineMediaTypeVideoAvi) {
    models::MediaFileModel file("/path/to/video.avi");
    EXPECT_EQ(file.getType(), models::MediaType::UNSUPPORTED);
}

TEST_F(MediaFileModelTest, CheckCaseSensitivity) {
    models::MediaFileModel file1("song.wav");
    EXPECT_EQ(file1.getType(), models::MediaType::AUDIO);
    
    models::MediaFileModel file2("SONG.WAV");
    // Uppercase extensions are UNSUPPORTED as per implementation
    EXPECT_EQ(file2.getType(), models::MediaType::UNSUPPORTED); 
}

TEST_F(MediaFileModelTest, UnsupportedExtension) {
    models::MediaFileModel file("/path/to/file.txt");
    EXPECT_EQ(file.getType(), models::MediaType::UNKNOWN);
}

// ===================== Properties =====================

TEST_F(MediaFileModelTest, SetGetProperties) {
    models::MediaFileModel file;
    file.setTitle("Test Title");
    file.setArtist("Test Artist");
    file.setDuration(120);
    
    EXPECT_EQ(file.getTitle(), "Test Title");
    EXPECT_EQ(file.getArtist(), "Test Artist");
    EXPECT_EQ(file.getDuration(), 120);
}

TEST_F(MediaFileModelTest, SetGetAlbum) {
    models::MediaFileModel file;
    file.setAlbum("Test Album");
    EXPECT_EQ(file.getAlbum(), "Test Album");
}

// ===================== isValid =====================

TEST_F(MediaFileModelTest, IsValidTrue) {
    models::MediaFileModel file(testFile.string());
    EXPECT_TRUE(file.isValid());
}

TEST_F(MediaFileModelTest, IsValidFalseEmptyPath) {
    models::MediaFileModel file;
    EXPECT_FALSE(file.isValid());
}

TEST_F(MediaFileModelTest, IsValidFalseNonexistent) {
    models::MediaFileModel file("/nonexistent/file.mp3");
    EXPECT_FALSE(file.isValid());
}

// ===================== Operators =====================

TEST_F(MediaFileModelTest, OperatorLessThan) {
    models::MediaFileModel file1("/path/to/a.mp3");
    models::MediaFileModel file2("/path/to/b.mp3");
    
    EXPECT_TRUE(file1 < file2);
    EXPECT_FALSE(file2 < file1);
}

TEST_F(MediaFileModelTest, OperatorEqual) {
    models::MediaFileModel file1("/path/to/song.mp3");
    models::MediaFileModel file2("/path/to/song.mp3");
    models::MediaFileModel file3("/path/to/other.mp3");
    
    EXPECT_TRUE(file1 == file2);
    EXPECT_FALSE(file1 == file3);
}

// ===================== Serialize / Deserialize =====================

TEST_F(MediaFileModelTest, Serialize) {
    models::MediaFileModel file(testFile.string());
    std::string serialized = file.serialize();
    
    EXPECT_FALSE(serialized.empty());
    EXPECT_NE(serialized.find("test.mp3"), std::string::npos);
}

TEST_F(MediaFileModelTest, Deserialize) {
    models::MediaFileModel original(testFile.string());
    std::string serialized = original.serialize();
    
    models::MediaFileModel restored = models::MediaFileModel::deserialize(serialized);
    EXPECT_EQ(restored.getFilePath(), original.getFilePath());
}

TEST_F(MediaFileModelTest, DeserializeInvalid) {
    std::string invalidData = "no pipe here";
    models::MediaFileModel restored = models::MediaFileModel::deserialize(invalidData);
    EXPECT_TRUE(restored.getFilePath().empty());
}

// ===================== File Size / Last Modified =====================

TEST_F(MediaFileModelTest, FileSize) {
    models::MediaFileModel file(testFile.string());
    EXPECT_GT(file.getFileSize(), 0);
}

// ===================== getFileName / getExtension =====================

TEST_F(MediaFileModelTest, GetFileNameAndExtension) {
    models::MediaFileModel file("/path/to/music/song.mp3");
    EXPECT_EQ(file.getFileName(), "song.mp3");
    EXPECT_EQ(file.getExtension(), ".mp3");
}

TEST_F(MediaFileModelTest, ExtensionWithMultipleDots) {
    models::MediaFileModel file("/path/to/my.song.mp3");
    EXPECT_EQ(file.getFileName(), "my.song.mp3");
    EXPECT_EQ(file.getExtension(), ".mp3");
}
