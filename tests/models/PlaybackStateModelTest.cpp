#include <gtest/gtest.h>
#include "models/PlaybackStateModel.h"

using namespace media_player;

class PlaybackStateModelTest : public testing::Test {
protected:
    models::PlaybackStateModel model;
};

TEST_F(PlaybackStateModelTest, InitialState) {
    EXPECT_EQ(model.getState(), models::PlaybackState::STOPPED);
    EXPECT_EQ(model.getCurrentPosition(), 0);
    EXPECT_EQ(model.getTotalDuration(), 0);
    EXPECT_EQ(model.getVolume(), 70);
}

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

TEST_F(PlaybackStateModelTest, PositionAndDuration) {
    model.setTotalDuration(300);
    model.setCurrentPosition(150);
    
    EXPECT_EQ(model.getTotalDuration(), 300);
    EXPECT_EQ(model.getCurrentPosition(), 150);
    EXPECT_FLOAT_EQ(model.getProgressPercentage(), 50.0f);
}

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
