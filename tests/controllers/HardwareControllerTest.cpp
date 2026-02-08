#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include "controllers/HardwareController.h"
#include "mocks/MockSerialCommunication.h"
#include "models/PlaybackStateModel.h"

using namespace media_player;
using namespace testing;

class HardwareControllerTest : public Test {
protected:
    void SetUp() override {
        mockSerial = std::make_shared<test::MockSerialCommunication>();
        playbackState = std::make_shared<models::PlaybackStateModel>();
        controller = std::make_shared<controllers::HardwareController>(mockSerial, playbackState);
    }

    std::shared_ptr<test::MockSerialCommunication> mockSerial;
    std::shared_ptr<models::PlaybackStateModel> playbackState;
    std::shared_ptr<controllers::HardwareController> controller;
};

TEST_F(HardwareControllerTest, InitializeConnectsToPort) {
    // Expect open to be called if autoConnect fails first or we explicitly connect
    // HardwareController::initialize calls autoConnect first
    // If autoConnect fails (returns false), it tries default port
    
    // Setup expectation for isOpen check in autoConnect and connect
    EXPECT_CALL(*mockSerial, isOpen()).WillRepeatedly(Return(false));
    
    // autoConnect spawns a thread. This makes testing tricky.
    // However, initialize also calls connect fallback if autoConnect returns false.
    // autoConnect returns false immediately as it's async.
    
    // Then it calls connect(DEFAULT_PORT, BAUD)
    EXPECT_CALL(*mockSerial, open(_, 115200))
        .WillOnce(Return(true));
        
    EXPECT_TRUE(controller->initialize());
}

TEST_F(HardwareControllerTest, SendCurrentSongInfo) {
    EXPECT_CALL(*mockSerial, isOpen()).WillRepeatedly(Return(true));
    EXPECT_CALL(*mockSerial, sendData("SONG|Title|Artist\n")).WillOnce(Return(true));
    
    controller->sendCurrentSongInfo("Title", "Artist");
}

TEST_F(HardwareControllerTest, SendPlaybackState) {
    EXPECT_CALL(*mockSerial, isOpen()).WillRepeatedly(Return(true));
    EXPECT_CALL(*mockSerial, sendData("STATE|PLAYING\n")).WillOnce(Return(true));
    
    controller->sendPlaybackState(true);
}

TEST_F(HardwareControllerTest, SendPlaybackStatePaused) {
    EXPECT_CALL(*mockSerial, isOpen()).WillRepeatedly(Return(true));
    EXPECT_CALL(*mockSerial, sendData("STATE|PAUSED\n")).WillOnce(Return(true));
    controller->sendPlaybackState(false);
}

TEST_F(HardwareControllerTest, AutoConnectReturnsTrueWhenAlreadyConnected) {
    EXPECT_CALL(*mockSerial, isOpen()).WillRepeatedly(Return(true));
    EXPECT_TRUE(controller->autoConnect());
}

TEST_F(HardwareControllerTest, ConnectReturnsFalseWhenOpenFails) {
    EXPECT_CALL(*mockSerial, open(_, 115200)).WillOnce(Return(false));
    EXPECT_FALSE(controller->connect("/dev/ttyUSB0", 115200));
}

TEST_F(HardwareControllerTest, DisconnectCallsCloseWhenOpen) {
    EXPECT_CALL(*mockSerial, isOpen()).WillRepeatedly(Return(true));
    EXPECT_CALL(*mockSerial, close()).Times(testing::AtLeast(1));
    controller->disconnect();
}

TEST_F(HardwareControllerTest, RefreshThrottleAndConnectedEarlyReturn) {
    EXPECT_CALL(*mockSerial, isOpen()).WillOnce(Return(true));
    controller->refreshConnection();
    EXPECT_CALL(*mockSerial, isOpen()).WillRepeatedly(Return(false));
    controller->refreshConnection();
}

TEST_F(HardwareControllerTest, InitializeFallbackUsesDefaultPortWhenAutoConnectFalse) {
    EXPECT_CALL(*mockSerial, isOpen()).WillRepeatedly(Return(false));
    EXPECT_CALL(*mockSerial, open("/dev/ttyUSB0", 115200)).WillOnce(Return(true));
    EXPECT_TRUE(controller->initialize());
}

TEST_F(HardwareControllerTest, SendSongInfoNoOpWhenDisconnected) {
    EXPECT_CALL(*mockSerial, isOpen()).WillRepeatedly(Return(false));
    controller->sendCurrentSongInfo("A", "B");
}

TEST_F(HardwareControllerTest, SendPlaybackStateNoOpWhenDisconnected) {
    EXPECT_CALL(*mockSerial, isOpen()).WillRepeatedly(Return(false));
    controller->sendPlaybackState(true);
}
