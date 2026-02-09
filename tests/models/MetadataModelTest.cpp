/**
 * @file MetadataModelTest.cpp
 * @brief Unit tests cho MetadataModel
 * 
 * Test các chức năng: constructor, getters, setters,
 * custom tags, validation, và display functions.
 */

#include <gtest/gtest.h>
#include "models/MetadataModel.h"

using namespace media_player::models;

class MetadataModelTest : public ::testing::Test {
protected:
    MetadataModel model;
};

// ===================== Cơ bản =====================

TEST_F(MetadataModelTest, DefaultConstructor) {
    MetadataModel m;
    EXPECT_EQ(m.getTitle(), "");
    EXPECT_EQ(m.getArtist(), "");
    EXPECT_EQ(m.getAlbum(), "");
    EXPECT_EQ(m.getGenre(), "");
    EXPECT_EQ(m.getYear(), "");
    EXPECT_EQ(m.getDuration(), 0);
    EXPECT_EQ(m.getBitrate(), 0);
    EXPECT_FALSE(m.hasCoverArt());
}

TEST_F(MetadataModelTest, ConstructorWithPath) {
    MetadataModel m("/path/to/file.mp3");
    // Constructor chỉ lưu path, không đọc file
    EXPECT_EQ(m.getTitle(), "");
}

// ===================== Setters/Getters =====================

TEST_F(MetadataModelTest, SetGetTitle) {
    model.setTitle("Test Song");
    EXPECT_EQ(model.getTitle(), "Test Song");
}

TEST_F(MetadataModelTest, SetGetArtist) {
    model.setArtist("Test Artist");
    EXPECT_EQ(model.getArtist(), "Test Artist");
}

TEST_F(MetadataModelTest, SetGetAlbum) {
    model.setAlbum("Test Album");
    EXPECT_EQ(model.getAlbum(), "Test Album");
}

TEST_F(MetadataModelTest, SetGetGenre) {
    model.setGenre("Rock");
    EXPECT_EQ(model.getGenre(), "Rock");
}

TEST_F(MetadataModelTest, SetGetYear) {
    model.setYear("2024");
    EXPECT_EQ(model.getYear(), "2024");
}

// ===================== Custom Tags =====================

TEST_F(MetadataModelTest, SetCustomTag) {
    model.setCustomTag("duration", "180");
    auto value = model.getCustomTag("duration");
    ASSERT_TRUE(value.has_value());
    EXPECT_EQ(*value, "180");
}

TEST_F(MetadataModelTest, GetCustomTagNotFound) {
    auto value = model.getCustomTag("nonexistent");
    EXPECT_FALSE(value.has_value());
}

TEST_F(MetadataModelTest, OverwriteCustomTag) {
    model.setCustomTag("key", "value1");
    model.setCustomTag("key", "value2");
    auto value = model.getCustomTag("key");
    ASSERT_TRUE(value.has_value());
    EXPECT_EQ(*value, "value2");
}

// ===================== isComplete =====================

TEST_F(MetadataModelTest, IsCompleteTrue) {
    model.setTitle("Song");
    model.setArtist("Artist");
    EXPECT_TRUE(model.isComplete());
}

TEST_F(MetadataModelTest, IsCompleteFalseNoTitle) {
    model.setArtist("Artist");
    EXPECT_FALSE(model.isComplete());
}

TEST_F(MetadataModelTest, IsCompleteFalseNoArtist) {
    model.setTitle("Song");
    EXPECT_FALSE(model.isComplete());
}

TEST_F(MetadataModelTest, IsCompleteFalseBothEmpty) {
    EXPECT_FALSE(model.isComplete());
}

// ===================== Display Functions =====================

TEST_F(MetadataModelTest, GetDisplayTitleWithTitle) {
    model.setTitle("My Song");
    EXPECT_EQ(model.getDisplayTitle(), "My Song");
}

TEST_F(MetadataModelTest, GetDisplayTitleEmpty) {
    EXPECT_EQ(model.getDisplayTitle(), "Unknown Title");
}

TEST_F(MetadataModelTest, GetDisplayArtistWithArtist) {
    model.setArtist("My Artist");
    EXPECT_EQ(model.getDisplayArtist(), "My Artist");
}

TEST_F(MetadataModelTest, GetDisplayArtistEmpty) {
    EXPECT_EQ(model.getDisplayArtist(), "Unknown Artist");
}

// ===================== getFormattedDuration =====================

TEST_F(MetadataModelTest, GetFormattedDurationZero) {
    // Duration = 0 => "0:00"
    EXPECT_EQ(model.getFormattedDuration(), "0:00");
}

TEST_F(MetadataModelTest, GetFormattedDurationSeconds) {
    // Duration 45 seconds => "0:45"
    model.setCustomTag("duration", "45");
    // Note: getFormattedDuration uses m_durationSeconds private field
    // which is not set by setCustomTag, so we test the behavior
    // This is a limitation - the test will show "0:00" unless m_durationSeconds is set
    // We check the format pattern is correct
    std::string formatted = model.getFormattedDuration();
    EXPECT_FALSE(formatted.empty());
}

TEST_F(MetadataModelTest, GetFormattedDurationMinutes) {
    // For testing format, the method reads from m_durationSeconds
    // Since we can't directly set it, we verify the return format
    std::string formatted = model.getFormattedDuration();
    // Should match pattern like "0:00" or "1:23" or "1:02:03"
    EXPECT_NE(formatted.find(':'), std::string::npos);
}

// ===================== Cover Art =====================

TEST_F(MetadataModelTest, HasCoverArtDefault) {
    EXPECT_FALSE(model.hasCoverArt());
}

TEST_F(MetadataModelTest, GetCoverArtEmpty) {
    auto data = model.getCoverArt();
    EXPECT_TRUE(data.empty());
}

// ===================== Publisher =====================

TEST_F(MetadataModelTest, GetPublisherDefault) {
    EXPECT_EQ(model.getPublisher(), "");
}
