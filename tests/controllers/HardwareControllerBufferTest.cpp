#include <gtest/gtest.h>
#include "controllers/HardwareController.h"
#include "models/PlaybackStateModel.h"
#include "services/ISerialCommunication.h"

using namespace media_player;

class DummySerialBuf : public services::ISerialCommunication {
public:
    bool open(const std::string& portName, int baudRate) override { (void)portName; (void)baudRate; opened = true; return true; }
    void close() override { opened = false; }
    bool isOpen() const override { return opened; }
    bool sendData(const std::string& data) override { sentMessages.push_back(data); return true; }
    std::string readData() override { return ""; }
    void setDataCallback(services::SerialDataCallback callback) override { dataCb = callback; }
    void setErrorCallback(services::SerialErrorCallback callback) override { errCb = callback; }
    bool opened = true;
    services::SerialDataCallback dataCb;
    services::SerialErrorCallback errCb;
    std::vector<std::string> sentMessages;
};

TEST(HardwareControllerBufferTest, PartialAggregationAndCompletion) {
    auto serial = std::make_shared<DummySerialBuf>();
    auto playbackState = std::make_shared<models::PlaybackStateModel>();
    controllers::HardwareController controller(serial, playbackState);
    int vol = -1;
    controller.setVolumeCallback([&](int v){ vol = v; });
    ASSERT_TRUE(serial->dataCb);
    serial->dataCb("!ADC:5");
    EXPECT_EQ(vol, -1);
    serial->dataCb("0!");
    EXPECT_EQ(vol, 50);
}

TEST(HardwareControllerBufferTest, SendMessagesEarlyReturnWhenClosed) {
    auto serial = std::make_shared<DummySerialBuf>();
    serial->opened = false;
    auto playbackState = std::make_shared<models::PlaybackStateModel>();
    controllers::HardwareController controller(serial, playbackState);
    controller.sendCurrentSongInfo("t", "a");
    controller.sendPlaybackState(true);
    EXPECT_TRUE(serial->sentMessages.empty());
}

TEST(HardwareControllerBufferTest, IncompleteMessageRetainedUntilCompleted) {
    auto serial = std::make_shared<DummySerialBuf>();
    auto playbackState = std::make_shared<models::PlaybackStateModel>();
    controllers::HardwareController controller(serial, playbackState);
    int vol = -1;
    controller.setVolumeCallback([&](int v){ vol = v; });
    ASSERT_TRUE(serial->dataCb);
    serial->dataCb("!ADC:10");
    EXPECT_EQ(vol, -1);
    serial->dataCb("!");
    EXPECT_EQ(vol, 10);
}

TEST(HardwareControllerBufferTest, GarbageBeforeMessageDoesNotPreventParsing) {
    auto serial = std::make_shared<DummySerialBuf>();
    auto playbackState = std::make_shared<models::PlaybackStateModel>();
    controllers::HardwareController controller(serial, playbackState);
    int btn = -1;
    controller.setButtonCallback([&](controllers::HardwareButton b){ btn = static_cast<int>(b); });
    ASSERT_TRUE(serial->dataCb);
    serial->dataCb("garbage");
    serial->dataCb("!BTN: 1 !");
    EXPECT_EQ(btn, 1);
}

TEST(HardwareControllerBufferTest, InvalidAdcValueIgnored) {
    auto serial = std::make_shared<DummySerialBuf>();
    auto playbackState = std::make_shared<models::PlaybackStateModel>();
    controllers::HardwareController controller(serial, playbackState);
    int vol = -1;
    controller.setVolumeCallback([&](int v){ vol = v; });
    ASSERT_TRUE(serial->dataCb);
    serial->dataCb("!ADC:101!");
    EXPECT_EQ(vol, -1);
}

TEST(HardwareControllerBufferTest, InvalidButtonIdIgnored) {
    auto serial = std::make_shared<DummySerialBuf>();
    auto playbackState = std::make_shared<models::PlaybackStateModel>();
    controllers::HardwareController controller(serial, playbackState);
    int btn = -1;
    controller.setButtonCallback([&](controllers::HardwareButton b){ btn = static_cast<int>(b); });
    ASSERT_TRUE(serial->dataCb);
    serial->dataCb("!BTN: 5 !");
    EXPECT_EQ(btn, -1);
}
