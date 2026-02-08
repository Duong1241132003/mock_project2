#include <gtest/gtest.h>
#include "controllers/HardwareController.h"
#include "models/PlaybackStateModel.h"
#include "services/ISerialCommunication.h"

using namespace media_player;

class SimpleSerial : public services::ISerialCommunication {
public:
    bool open(const std::string& portName, int baudRate) override { (void)portName; (void)baudRate; opened = true; return true; }
    void close() override { opened = false; }
    bool isOpen() const override { return opened; }
    bool sendData(const std::string& data) override { (void)data; return true; }
    std::string readData() override { return ""; }
    void setDataCallback(services::SerialDataCallback callback) override { dataCb = callback; }
    void setErrorCallback(services::SerialErrorCallback callback) override { errCb = callback; }
    bool opened = true;
    services::SerialDataCallback dataCb;
    services::SerialErrorCallback errCb;
};

TEST(HardwareControllerParseTest, BtnValidTriggersCallback) {
    auto serial = std::make_shared<SimpleSerial>();
    auto playbackState = std::make_shared<models::PlaybackStateModel>();
    controllers::HardwareController controller(serial, playbackState);
    int got = -1;
    controller.setButtonCallback([&](controllers::HardwareButton b){ got = static_cast<int>(b); });
    ASSERT_TRUE(serial->dataCb);
    serial->dataCb("!BTN: 2 !");
    EXPECT_EQ(got, 2);
}

TEST(HardwareControllerParseTest, BtnInvalidIgnored) {
    auto serial = std::make_shared<SimpleSerial>();
    auto playbackState = std::make_shared<models::PlaybackStateModel>();
    controllers::HardwareController controller(serial, playbackState);
    int got = -1;
    controller.setButtonCallback([&](controllers::HardwareButton b){ got = static_cast<int>(b); });
    ASSERT_TRUE(serial->dataCb);
    serial->dataCb("!BTN: 99 !");
    EXPECT_EQ(got, -1);
}

TEST(HardwareControllerParseTest, AdcBoundariesAndSpaces) {
    auto serial = std::make_shared<SimpleSerial>();
    auto playbackState = std::make_shared<models::PlaybackStateModel>();
    controllers::HardwareController controller(serial, playbackState);
    int vol = -1;
    controller.setVolumeCallback([&](int v){ vol = v; });
    ASSERT_TRUE(serial->dataCb);
    serial->dataCb("!ADC:   0   !");
    EXPECT_EQ(vol, 0);
    serial->dataCb("!ADC: 100 !");
    EXPECT_EQ(vol, 100);
}

TEST(HardwareControllerParseTest, BtnOutOfRangeIgnored) {
    auto serial = std::make_shared<SimpleSerial>();
    auto playbackState = std::make_shared<models::PlaybackStateModel>();
    controllers::HardwareController controller(serial, playbackState);
    int got = -1;
    controller.setButtonCallback([&](controllers::HardwareButton b){ got = static_cast<int>(b); });
    ASSERT_TRUE(serial->dataCb);
    serial->dataCb("!BTN: 5 !");
    EXPECT_EQ(got, -1);
}

TEST(HardwareControllerParseTest, MultipleMessagesParsedInSingleBuffer) {
    auto serial = std::make_shared<SimpleSerial>();
    auto playbackState = std::make_shared<models::PlaybackStateModel>();
    controllers::HardwareController controller(serial, playbackState);
    int vol = -1;
    int btn = -1;
    controller.setVolumeCallback([&](int v){ vol = v; });
    controller.setButtonCallback([&](controllers::HardwareButton b){ btn = static_cast<int>(b); });
    ASSERT_TRUE(serial->dataCb);
    serial->dataCb("!ADC: 15!!BTN: 4 !");
    EXPECT_EQ(vol, 15);
    EXPECT_EQ(btn, 4);
}
TEST(HardwareControllerParseTest, BufferClearsOnOverflow) {
    auto serial = std::make_shared<SimpleSerial>();
    auto playbackState = std::make_shared<models::PlaybackStateModel>();
    controllers::HardwareController controller(serial, playbackState);
    std::string big(1100, 'x');
    ASSERT_TRUE(serial->dataCb);
    serial->dataCb(big);
    serial->dataCb("!ADC: 10 !");
}
