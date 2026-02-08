#include <gtest/gtest.h>
#include "controllers/HardwareController.h"
#include "models/PlaybackStateModel.h"
#include "services/ISerialCommunication.h"

using namespace media_player;

class AlwaysOpenSerial : public services::ISerialCommunication {
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

TEST(HardwareControllerRefreshTest, RefreshNoOpWhenConnected) {
    auto serial = std::make_shared<AlwaysOpenSerial>();
    auto playbackState = std::make_shared<models::PlaybackStateModel>();
    controllers::HardwareController controller(serial, playbackState);
    EXPECT_TRUE(serial->isOpen());
    controller.refreshConnection();
    serial->close();
    controller.refreshConnection();
}
