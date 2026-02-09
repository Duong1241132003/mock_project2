#include <gtest/gtest.h>
#include "models/PlaybackStateModel.h"

using namespace media_player;

class PlaybackStateModelTest : public testing::Test {
protected:
    models::PlaybackStateModel model;
};

// ===================== Initial State =====================

TEST_F(PlaybackStateModelTest, InitialState) {
    EXPECT_EQ(model.getState(), models::PlaybackState::STOPPED);
    EXPECT_EQ(model.getCurrentPosition(), 0);
    EXPECT_EQ(model.getTotalDuration(), 0);
    EXPECT_EQ(model.getVolume(), 70);
    EXPECT_TRUE(model.getCurrentFilePath().empty());
    EXPECT_TRUE(model.getCurrentTitle().empty());
    EXPECT_TRUE(model.getCurrentArtist().empty());
}

// ===================== State Transitions =====================

TEST_F(PlaybackStateModelTest, StateTransitions) {
    model.setState(models::PlaybackState::PLAYING);
    EXPECT_EQ(model.getState(), models::PlaybackState::PLAYING);
    EXPECT_TRUE(model.isPlaying());
    EXPECT_FALSE(model.isPaused());
    EXPECT_FALSE(model.isStopped());
    
    model.setState(models::PlaybackState::PAUSED);
    EXPECT_EQ(model.getState(), models::PlaybackState::PAUSED);
    EXPECT_FALSE(model.isPlaying());
    EXPECT_TRUE(model.isPaused());
    
    model.setState(models::PlaybackState::STOPPED);
    EXPECT_EQ(model.getState(), models::PlaybackState::STOPPED);
    EXPECT_TRUE(model.isStopped());
}

// ===================== Position and Duration =====================

TEST_F(PlaybackStateModelTest, PositionAndDuration) {
    model.setTotalDuration(300);
    model.setCurrentPosition(150);
    
    EXPECT_EQ(model.getTotalDuration(), 300);
    EXPECT_EQ(model.getCurrentPosition(), 150);
    EXPECT_FLOAT_EQ(model.getProgressPercentage(), 50.0f);
}

TEST_F(PlaybackStateModelTest, ProgressPercentageZeroDuration) {
    // Khi duration = 0, progress = 0%
    model.setTotalDuration(0);
    model.setCurrentPosition(100);
    EXPECT_FLOAT_EQ(model.getProgressPercentage(), 0.0f);
}

// ===================== Callbacks =====================

TEST_F(PlaybackStateModelTest, Callbacks) {
    bool callbackCalled = false;
    models::PlaybackState receivedState = models::PlaybackState::STOPPED;
    
    model.setStateChangeCallback([&](models::PlaybackState state) {
        callbackCalled = true;
        receivedState = state;
    });
    
    model.setState(models::PlaybackState::PLAYING);
    
    EXPECT_TRUE(callbackCalled);
    EXPECT_EQ(receivedState, models::PlaybackState::PLAYING);
}

TEST_F(PlaybackStateModelTest, Metadata) {
    bool metadataCallbackCalled = false;
    model.setMetadataChangeCallback([&]() {
        metadataCallbackCalled = true;
    });
    
    model.setCurrentTitle("New Title");
    EXPECT_EQ(model.getCurrentTitle(), "New Title");
    EXPECT_TRUE(metadataCallbackCalled);
    
    metadataCallbackCalled = false;
    model.setCurrentArtist("New Artist");
    EXPECT_EQ(model.getCurrentArtist(), "New Artist");
    EXPECT_TRUE(metadataCallbackCalled);
}

// ===================== Formatted Position =====================

TEST_F(PlaybackStateModelTest, GetFormattedPositionZero) {
    model.setCurrentPosition(0);
    std::string formatted = model.getFormattedPosition();
    EXPECT_EQ(formatted, "00:00");
}

TEST_F(PlaybackStateModelTest, GetFormattedPositionSeconds) {
    model.setCurrentPosition(45);
    std::string formatted = model.getFormattedPosition();
    EXPECT_EQ(formatted, "00:45");
}

TEST_F(PlaybackStateModelTest, GetFormattedPositionMinutes) {
    model.setCurrentPosition(125); // 2:05
    std::string formatted = model.getFormattedPosition();
    EXPECT_EQ(formatted, "02:05");
}

TEST_F(PlaybackStateModelTest, GetFormattedPositionHours) {
    model.setCurrentPosition(3661); // 1:01:01
    std::string formatted = model.getFormattedPosition();
    EXPECT_EQ(formatted, "01:01:01");
}

// ===================== Formatted Duration =====================

TEST_F(PlaybackStateModelTest, GetFormattedDurationZero) {
    model.setTotalDuration(0);
    std::string formatted = model.getFormattedDuration();
    EXPECT_EQ(formatted, "00:00");
}

TEST_F(PlaybackStateModelTest, GetFormattedDurationMinutes) {
    model.setTotalDuration(180); // 3:00
    std::string formatted = model.getFormattedDuration();
    EXPECT_EQ(formatted, "03:00");
}

TEST_F(PlaybackStateModelTest, GetFormattedDurationHours) {
    model.setTotalDuration(7200); // 2:00:00
    std::string formatted = model.getFormattedDuration();
    EXPECT_EQ(formatted, "02:00:00");
}

// ===================== Volume =====================

TEST_F(PlaybackStateModelTest, SetVolumeNormal) {
    model.setVolume(50);
    EXPECT_EQ(model.getVolume(), 50);
}

TEST_F(PlaybackStateModelTest, SetVolumeMin) {
    model.setVolume(0);
    EXPECT_EQ(model.getVolume(), 0);
}

TEST_F(PlaybackStateModelTest, SetVolumeMax) {
    model.setVolume(100);
    EXPECT_EQ(model.getVolume(), 100);
}

// ===================== File Path =====================

TEST_F(PlaybackStateModelTest, SetGetCurrentFilePath) {
    model.setCurrentFilePath("/path/to/music.mp3");
    EXPECT_EQ(model.getCurrentFilePath(), "/path/to/music.mp3");
}

