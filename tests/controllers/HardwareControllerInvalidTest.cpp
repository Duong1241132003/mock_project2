#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include "controllers/HardwareController.h"
#include "models/PlaybackStateModel.h"
#include "services/ISerialCommunication.h"

using namespace media_player;
using ::testing::_;

class MockSerialComm2 : public services::ISerialCommunication {
public:
    MOCK_METHOD(bool, open, (const std::string& portName, int baudRate), (override));
    MOCK_METHOD(void, close, (), (override));
    MOCK_METHOD(bool, isOpen, (), (const, override));
    MOCK_METHOD(bool, sendData, (const std::string& data), (override));
    MOCK_METHOD(std::string, readData, (), (override));
    MOCK_METHOD(void, setDataCallback, (services::SerialDataCallback callback), (override));
    MOCK_METHOD(void, setErrorCallback, (services::SerialErrorCallback callback), (override));
};

TEST(HardwareControllerInvalidTest, IgnoreInvalidMessagesAndOutOfRange) {
    auto serial = std::make_shared<MockSerialComm2>();
    std::function<void(const std::string&)> cb;
    EXPECT_CALL(*serial, setDataCallback(_)).WillOnce(testing::Invoke([&](auto f){ cb = f; }));
    auto playbackState = std::make_shared<models::PlaybackStateModel>();
    controllers::HardwareController controller(serial, playbackState);
    int vol = -1;
    int btn = -1;
    controller.setVolumeCallback([&](int v){ vol = v; });
    controller.setButtonCallback([&](controllers::HardwareButton b){ btn = static_cast<int>(b); });
    ASSERT_TRUE(cb);
    cb("BAD");
    cb("!ADC:200!");
    cb("!BTN:0!");
    EXPECT_EQ(vol, -1);
    EXPECT_EQ(btn, -1);
}
