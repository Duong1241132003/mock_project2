#include <gtest/gtest.h>
#include <fstream>
#include "controllers/MainController.h"
#include "controllers/PlaybackController.h"
#include "controllers/HardwareController.h"
#include "models/QueueModel.h"
#include "models/PlaybackStateModel.h"
#include "models/SystemStateModel.h"
#include "repositories/HistoryRepository.h"
#include "models/HistoryModel.h"
#include "services/ISerialCommunication.h"

using namespace media_player;

class DummySerial : public services::ISerialCommunication {
public:
    bool open(const std::string& portName, int baudRate) override { return true; }
    void close() override {}
    bool isOpen() const override { return true; }
    bool sendData(const std::string& data) override { return true; }
    std::string readData() override { return ""; }
    void setDataCallback(services::SerialDataCallback callback) override {}
    void setErrorCallback(services::SerialErrorCallback callback) override {}
};

TEST(MainControllerTest, NavigationInitializeShutdown) {
    auto queueModel = std::make_shared<models::QueueModel>();
    auto playbackState = std::make_shared<models::PlaybackStateModel>();
    auto historyRepo = std::make_shared<repositories::HistoryRepository>("/tmp/hist_main");
    auto historyModel = std::make_shared<models::HistoryModel>(historyRepo);
    auto playbackController = std::make_shared<controllers::PlaybackController>(queueModel, playbackState, historyModel);
    auto serial = std::make_shared<DummySerial>();
    auto hwPlaybackState = std::make_shared<models::PlaybackStateModel>();
    auto hardwareController = std::make_shared<controllers::HardwareController>(serial, hwPlaybackState);
    auto systemState = std::make_shared<models::SystemStateModel>();
    auto main = std::make_shared<controllers::MainController>(
        playbackController, nullptr, nullptr, nullptr, nullptr, hardwareController, systemState);
    main->navigateTo(controllers::ScreenType::LIBRARY);
    EXPECT_EQ(main->getCurrentScreen(), controllers::ScreenType::LIBRARY);
    main->navigateTo(controllers::ScreenType::PLAYLIST);
    main->navigateTo(controllers::ScreenType::QUEUE);
    main->navigateTo(controllers::ScreenType::SCAN);
    EXPECT_TRUE(main->initialize());
    main->handleGlobalKeyPress(27);
    main->handleGlobalEvent("PING");
    main->shutdown();
}
