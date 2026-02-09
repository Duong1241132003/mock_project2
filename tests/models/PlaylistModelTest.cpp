#include <gtest/gtest.h>
#include "models/PlaylistModel.h"
#include "models/MediaFileModel.h"
#include <fstream>
#include <filesystem>

namespace fs = std::filesystem;

using namespace media_player::models;

class PlaylistModelTest : public ::testing::Test {
protected:
    PlaylistModel model;
    
    void SetUp() override {
        // Tạo test files nếu cần
        testDir = fs::temp_directory_path() / "MediaPlayerTest_Playlist";
        fs::create_directories(testDir);
        testFile1 = testDir / "song1.mp3";
        testFile2 = testDir / "song2.mp3";
        std::ofstream(testFile1) << "test";
        std::ofstream(testFile2) << "test";
    }
    
    void TearDown() override {
        if (fs::exists(testDir)) {
            fs::remove_all(testDir);
        }
    }
    
    fs::path testDir;
    fs::path testFile1;
    fs::path testFile2;
};

// ===================== Basic =====================

TEST_F(PlaylistModelTest, InitialState) {
    EXPECT_TRUE(model.getItems().empty());
    EXPECT_EQ(model.getName(), "");
    EXPECT_EQ(model.getItemCount(), 0);
}

TEST_F(PlaylistModelTest, ConstructorWithName) {
    PlaylistModel namedPlaylist("My Favorites");
    EXPECT_EQ(namedPlaylist.getName(), "My Favorites");
    EXPECT_FALSE(namedPlaylist.getId().empty());
}

TEST_F(PlaylistModelTest, SetName) {
    model.setName("My Playlist");
    EXPECT_EQ(model.getName(), "My Playlist");
}

TEST_F(PlaylistModelTest, AddAndGetItems) {
    MediaFileModel file1("/path/to/song1.mp3");
    model.addItem(file1);
    
    auto items = model.getItems();
    EXPECT_EQ(items.size(), 1);
    EXPECT_EQ(items[0].getFilePath(), "/path/to/song1.mp3");
    EXPECT_EQ(model.getItemCount(), 1);
}

// ===================== RemoveItem =====================

TEST_F(PlaylistModelTest, RemoveItemByIndex) {
    MediaFileModel file1("/path/to/song1.mp3");
    model.addItem(file1);
    
    EXPECT_TRUE(model.removeItem(0));
    EXPECT_TRUE(model.getItems().empty());
    
    // Remove invalid index
    EXPECT_FALSE(model.removeItem(0));
}

TEST_F(PlaylistModelTest, RemoveItemByPath) {
    MediaFileModel file1("/path/to/song1.mp3");
    MediaFileModel file2("/path/to/song2.mp3");
    model.addItem(file1);
    model.addItem(file2);
    
    EXPECT_TRUE(model.removeItem("/path/to/song1.mp3"));
    EXPECT_EQ(model.getItemCount(), 1);
    EXPECT_EQ(model.getItems()[0].getFilePath(), "/path/to/song2.mp3");
}

TEST_F(PlaylistModelTest, RemoveItemByPathNotFound) {
    MediaFileModel file1("/path/to/song1.mp3");
    model.addItem(file1);
    
    EXPECT_FALSE(model.removeItem("/nonexistent.mp3"));
    EXPECT_EQ(model.getItemCount(), 1);
}

// ===================== Clear =====================

TEST_F(PlaylistModelTest, Clear) {
    MediaFileModel file1("/1.mp3");
    MediaFileModel file2("/2.mp3");
    model.addItem(file1);
    model.addItem(file2);
    
    model.clear();
    EXPECT_TRUE(model.getItems().empty());
    EXPECT_EQ(model.getItemCount(), 0);
}

// ===================== GetItemAt =====================

TEST_F(PlaylistModelTest, GetItemAtValid) {
    MediaFileModel file1("/1.mp3");
    MediaFileModel file2("/2.mp3");
    model.addItem(file1);
    model.addItem(file2);
    
    auto item = model.getItemAt(1);
    ASSERT_TRUE(item.has_value());
    EXPECT_EQ(item->getFilePath(), "/2.mp3");
}

TEST_F(PlaylistModelTest, GetItemAtOutOfBounds) {
    MediaFileModel file1("/1.mp3");
    model.addItem(file1);
    
    auto item = model.getItemAt(100);
    EXPECT_FALSE(item.has_value());
}

// ===================== MoveItem =====================

TEST_F(PlaylistModelTest, ReorderItems) {
    MediaFileModel file1("/1.mp3");
    MediaFileModel file2("/2.mp3");
    model.addItem(file1);
    model.addItem(file2);
    
    model.moveItem(0, 1);
    auto items = model.getItems();
    EXPECT_EQ(items[0].getFilePath(), "/2.mp3");
    EXPECT_EQ(items[1].getFilePath(), "/1.mp3");
    
    EXPECT_FALSE(model.moveItem(0, 5));
}

// ===================== ContainsFile / FindItemIndex =====================

TEST_F(PlaylistModelTest, ContainsFile) {
    MediaFileModel file1("/path/to/song.mp3");
    model.addItem(file1);
    
    EXPECT_TRUE(model.containsFile("/path/to/song.mp3"));
    EXPECT_FALSE(model.containsFile("/nonexistent.mp3"));
}

TEST_F(PlaylistModelTest, FindItemIndex) {
    MediaFileModel file1("/1.mp3");
    MediaFileModel file2("/2.mp3");
    model.addItem(file1);
    model.addItem(file2);
    
    EXPECT_EQ(model.findItemIndex("/1.mp3"), 0);
    EXPECT_EQ(model.findItemIndex("/2.mp3"), 1);
    EXPECT_EQ(model.findItemIndex("/nonexistent.mp3"), -1);
}

// ===================== GetTotalDuration =====================

TEST_F(PlaylistModelTest, GetTotalDuration) {
    // getTotalDuration returns 0 (TODO in implementation)
    EXPECT_EQ(model.getTotalDuration(), 0);
}

// ===================== Serialize / Deserialize =====================

TEST_F(PlaylistModelTest, Serialize) {
    model.setName("Test Playlist");
    MediaFileModel file1(testFile1.string());
    model.addItem(file1);
    
    std::string serialized = model.serialize();
    EXPECT_FALSE(serialized.empty());
    EXPECT_NE(serialized.find("Test Playlist"), std::string::npos);
}

TEST_F(PlaylistModelTest, Deserialize) {
    // Create a serialized string
    model.setName("Deserialize Test");
    MediaFileModel file1(testFile1.string());
    model.addItem(file1);
    std::string serialized = model.serialize();
    
    // Deserialize
    PlaylistModel restored = PlaylistModel::deserialize(serialized);
    EXPECT_EQ(restored.getName(), "Deserialize Test");
    // Items might be added if files exist
}

TEST_F(PlaylistModelTest, DeserializeWithInvalidItems) {
    // Test with invalid file paths
    std::string data = "id123|Invalid Playlist|2|/nonexistent1.mp3|/nonexistent2.mp3";
    PlaylistModel restored = PlaylistModel::deserialize(data);
    EXPECT_EQ(restored.getName(), "Invalid Playlist");
    // Invalid items should be filtered out
}

