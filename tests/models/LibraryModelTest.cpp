#include <gtest/gtest.h>
#include "models/LibraryModel.h"
#include "models/MediaFileModel.h"
#include <filesystem>
#include <fstream>

namespace fs = std::filesystem;
using namespace media_player::models;

class LibraryModelTest : public ::testing::Test {
protected:
    LibraryModel model;
    
    void SetUp() override {
        testDir = fs::temp_directory_path() / "LibraryModelTest";
        fs::create_directories(testDir);
        
        // Create actual test files
        audioFile = testDir / "song.mp3";
        audioFile2 = testDir / "another.mp3";
        videoFile = testDir / "video.mp4";
        std::ofstream(audioFile) << "audio data";
        std::ofstream(audioFile2) << "audio data";
        std::ofstream(videoFile) << "video data";
    }
    
    void TearDown() override {
        if (fs::exists(testDir)) {
            fs::remove_all(testDir);
        }
    }
    
    fs::path testDir;
    fs::path audioFile;
    fs::path audioFile2;
    fs::path videoFile;
};

// ===================== Basic =====================

TEST_F(LibraryModelTest, InitialState) {
    EXPECT_TRUE(model.isEmpty());
    EXPECT_EQ(model.getMediaCount(), 0);
}

TEST_F(LibraryModelTest, AddAndGetFiles) {
    MediaFileModel file1("/path/to/song1.mp3");
    MediaFileModel file2("/path/to/song2.mp3");
    
    model.addMedia(file1);
    model.addMedia(file2);
    
    auto files = model.getAllMedia();
    EXPECT_EQ(files.size(), 2);
    EXPECT_EQ(files[0].getFilePath(), "/path/to/song1.mp3");
    EXPECT_EQ(files[1].getFilePath(), "/path/to/song2.mp3");
}

TEST_F(LibraryModelTest, AddDuplicate) {
    MediaFileModel file1("/path/to/song1.mp3");
    
    model.addMedia(file1);
    model.addMedia(file1); // Duplicate - should be ignored
    
    EXPECT_EQ(model.getMediaCount(), 1);
}

TEST_F(LibraryModelTest, Clear) {
    MediaFileModel file1("/path/to/song1.mp3");
    model.addMedia(file1);
    EXPECT_EQ(model.getMediaCount(), 1);
    
    model.clear();
    EXPECT_TRUE(model.isEmpty());
}

// ===================== AddMediaBatch =====================

TEST_F(LibraryModelTest, AddMediaBatch) {
    std::vector<MediaFileModel> batch = {
        MediaFileModel("/path/to/song1.mp3"),
        MediaFileModel("/path/to/song2.mp3"),
        MediaFileModel("/path/to/song3.mp3")
    };
    
    model.addMediaBatch(batch);
    EXPECT_EQ(model.getMediaCount(), 3);
}

TEST_F(LibraryModelTest, AddMediaBatchWithDuplicates) {
    MediaFileModel file1("/path/to/song1.mp3");
    model.addMedia(file1);
    
    std::vector<MediaFileModel> batch = {
        MediaFileModel("/path/to/song1.mp3"), // Duplicate
        MediaFileModel("/path/to/song2.mp3")
    };
    
    model.addMediaBatch(batch);
    EXPECT_EQ(model.getMediaCount(), 2); // 1 original + 1 new
}

// ===================== RemoveMedia =====================

TEST_F(LibraryModelTest, RemoveFile) {
    MediaFileModel file1("/path/to/song1.mp3");
    MediaFileModel file2("/path/to/song2.mp3");
    model.addMedia(file1);
    model.addMedia(file2);
    
    EXPECT_TRUE(model.removeMedia("/path/to/song1.mp3"));
    auto files = model.getAllMedia();
    EXPECT_EQ(files.size(), 1);
    EXPECT_EQ(files[0].getFilePath(), "/path/to/song2.mp3");
    
    // Remove non-existent
    EXPECT_FALSE(model.removeMedia("/non/existent.mp3"));
    EXPECT_EQ(model.getMediaCount(), 1);
}

// ===================== UpdateMedia =====================

TEST_F(LibraryModelTest, UpdateMedia) {
    MediaFileModel file1("/path/to/song1.mp3");
    file1.setTitle("Original Title");
    model.addMedia(file1);
    
    MediaFileModel updated("/path/to/song1.mp3");
    updated.setTitle("Updated Title");
    
    EXPECT_TRUE(model.updateMedia("/path/to/song1.mp3", updated));
    
    auto found = model.getMediaByPath("/path/to/song1.mp3");
    ASSERT_TRUE(found.has_value());
    EXPECT_EQ(found->getTitle(), "Updated Title");
}

TEST_F(LibraryModelTest, UpdateMediaNotFound) {
    MediaFileModel updated("/path/to/nonexistent.mp3");
    EXPECT_FALSE(model.updateMedia("/path/to/nonexistent.mp3", updated));
}

// ===================== GetMediaByPath =====================

TEST_F(LibraryModelTest, FindFile) {
    MediaFileModel file1("/path/to/song1.mp3");
    model.addMedia(file1);
    
    auto found = model.getMediaByPath("/path/to/song1.mp3");
    EXPECT_TRUE(found.has_value());
    EXPECT_EQ(found->getFilePath(), "/path/to/song1.mp3");
    
    auto notFound = model.getMediaByPath("/path/to/song2.mp3");
    EXPECT_FALSE(notFound.has_value());
}

// ===================== Search =====================

TEST_F(LibraryModelTest, SearchByQuery) {
    MediaFileModel file1("/path/to/song1.mp3");
    MediaFileModel file2("/path/to/another.mp3");
    MediaFileModel file3("/path/to/video.mp4");
    model.addMedia(file1);
    model.addMedia(file2);
    model.addMedia(file3);
    
    auto results = model.search("song");
    EXPECT_EQ(results.size(), 1);
    EXPECT_EQ(results[0].getFilePath(), "/path/to/song1.mp3");
}

TEST_F(LibraryModelTest, SearchCaseInsensitive) {
    MediaFileModel file1("/path/to/MySong.mp3");
    model.addMedia(file1);
    
    auto results = model.search("mysong");
    EXPECT_EQ(results.size(), 1);
    
    results = model.search("MYSONG");
    EXPECT_EQ(results.size(), 1);
}

TEST_F(LibraryModelTest, SearchNoResults) {
    MediaFileModel file1("/path/to/song.mp3");
    model.addMedia(file1);
    
    auto results = model.search("xyz");
    EXPECT_TRUE(results.empty());
}

TEST_F(LibraryModelTest, SearchEmptyQueryReturnsAll) {
    model.addMedia(MediaFileModel("/path/to/song1.mp3"));
    model.addMedia(MediaFileModel("/path/to/song2.mp3"));
    auto results = model.search("");
    EXPECT_EQ(results.size(), 2);
}

// ===================== GetSorted =====================

TEST_F(LibraryModelTest, GetSortedByTitleAscending) {
    MediaFileModel file1("/path/to/b_song.mp3");
    MediaFileModel file2("/path/to/a_song.mp3");
    model.addMedia(file1);
    model.addMedia(file2);
    
    auto sorted = model.getSorted(SortCriteria::TITLE, true);
    EXPECT_EQ(sorted.size(), 2);
    EXPECT_EQ(sorted[0].getFileName(), "a_song.mp3");
    EXPECT_EQ(sorted[1].getFileName(), "b_song.mp3");
}

TEST_F(LibraryModelTest, GetSortedByTitleDescending) {
    MediaFileModel file1("/path/to/a_song.mp3");
    MediaFileModel file2("/path/to/b_song.mp3");
    model.addMedia(file1);
    model.addMedia(file2);
    
    auto sorted = model.getSorted(SortCriteria::TITLE, false);
    EXPECT_EQ(sorted.size(), 2);
    EXPECT_EQ(sorted[0].getFileName(), "b_song.mp3");
    EXPECT_EQ(sorted[1].getFileName(), "a_song.mp3");
}

TEST_F(LibraryModelTest, GetSortedByFileName) {
    MediaFileModel file1("/path/to/zfile.mp3");
    MediaFileModel file2("/path/to/afile.mp3");
    model.addMedia(file1);
    model.addMedia(file2);
    
    auto sorted = model.getSorted(SortCriteria::FILE_NAME, true);
    EXPECT_EQ(sorted[0].getFileName(), "afile.mp3");
}

TEST_F(LibraryModelTest, GetSortedByArtist) {
    MediaFileModel file1("/path/to/song1.mp3");
    MediaFileModel file2("/path/to/song2.mp3");
    model.addMedia(file1);
    model.addMedia(file2);
    
    auto sorted = model.getSorted(SortCriteria::ARTIST, true);
    EXPECT_EQ(sorted.size(), 2);
}

TEST_F(LibraryModelTest, GetSortedByAlbum) {
    MediaFileModel file1("/path/to/song1.mp3");
    model.addMedia(file1);
    
    auto sorted = model.getSorted(SortCriteria::ALBUM, true);
    EXPECT_EQ(sorted.size(), 1);
}

TEST_F(LibraryModelTest, GetSortedByDateAdded) {
    MediaFileModel file1("/path/to/song1.mp3");
    model.addMedia(file1);
    
    auto sorted = model.getSorted(SortCriteria::DATE_ADDED, false);
    EXPECT_EQ(sorted.size(), 1);
}

// ===================== GetPage =====================

TEST_F(LibraryModelTest, GetPage) {
    for (int i = 0; i < 25; i++) {
        model.addMedia(MediaFileModel("/path/to/song" + std::to_string(i) + ".mp3"));
    }
    
    auto page0 = model.getPage(0, 10);
    EXPECT_EQ(page0.size(), 10);
    
    auto page2 = model.getPage(2, 10);
    EXPECT_EQ(page2.size(), 5); // 25 items, page 2 has items 20-24
}

TEST_F(LibraryModelTest, GetPageOutOfBounds) {
    model.addMedia(MediaFileModel("/path/to/song.mp3"));
    
    auto page = model.getPage(100, 10);
    EXPECT_TRUE(page.empty());
}

TEST_F(LibraryModelTest, GetPageExactBoundary) {
    for (int i = 0; i < 10; i++) {
        model.addMedia(MediaFileModel("/path/to/song" + std::to_string(i) + ".mp3"));
    }
    
    auto page = model.getPage(0, 10);
    EXPECT_EQ(page.size(), 10);
    
    // Page 1 should be empty as we have exactly 10 items
    auto page1 = model.getPage(1, 10);
    EXPECT_TRUE(page1.empty());
}

TEST_F(LibraryModelTest, GetPageZeroItemsPerPage) {
    model.addMedia(MediaFileModel("/path/to/song.mp3"));
    auto page = model.getPage(0, 0);
    EXPECT_TRUE(page.empty());
}

// ===================== GetTotalAudioFiles / VideoFiles =====================

TEST_F(LibraryModelTest, GetTotalAudioFiles) {
    model.addMedia(MediaFileModel(audioFile.string()));
    model.addMedia(MediaFileModel(audioFile2.string()));
    model.addMedia(MediaFileModel(videoFile.string()));
    
    EXPECT_EQ(model.getTotalAudioFiles(), 2);
}

TEST_F(LibraryModelTest, GetTotalVideoFiles) {
    model.addMedia(MediaFileModel(audioFile.string()));
    model.addMedia(MediaFileModel(videoFile.string()));
    
    EXPECT_EQ(model.getTotalVideoFiles(), 0);
}

TEST_F(LibraryModelTest, GetTotalAudioFilesEmpty) {
    EXPECT_EQ(model.getTotalAudioFiles(), 0);
}

TEST_F(LibraryModelTest, GetTotalVideoFilesEmpty) {
    EXPECT_EQ(model.getTotalVideoFiles(), 0);
}

// ===================== GetTotalSize =====================

TEST_F(LibraryModelTest, GetTotalSize) {
    model.addMedia(MediaFileModel(audioFile.string()));
    model.addMedia(MediaFileModel(videoFile.string()));
    
    // Files should have some size
    EXPECT_GT(model.getTotalSize(), 0);
}

TEST_F(LibraryModelTest, GetTotalSizeEmpty) {
    EXPECT_EQ(model.getTotalSize(), 0);
}
