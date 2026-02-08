#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include "controllers/SourceController.h"
#include "models/LibraryModel.h"
#include "repositories/LibraryRepository.h"
#include "services/IFileScanner.h"

using namespace media_player;
using ::testing::_;

class MockLibraryRepository : public repositories::LibraryRepository {
public:
    MockLibraryRepository() : repositories::LibraryRepository("/tmp/lib") {}
    MOCK_METHOD(bool, save, (const models::MediaFileModel&), (override));
    MOCK_METHOD(std::optional<models::MediaFileModel>, findById, (const std::string&), (override));
    MOCK_METHOD(std::vector<models::MediaFileModel>, findAll, (), (override));
    MOCK_METHOD(bool, update, (const models::MediaFileModel&), (override));
    MOCK_METHOD(bool, remove, (const std::string&), (override));
    MOCK_METHOD(bool, exists, (const std::string&), (override));
    MOCK_METHOD(bool, saveAll, (const std::vector<models::MediaFileModel>&), (override));
    MOCK_METHOD(void, clear, (), (override));
    MOCK_METHOD(size_t, count, (), (const, override));
};

class MockScanner : public services::IFileScanner {
public:
    MOCK_METHOD(void, scanDirectory, (const std::string& rootPath), (override));
    MOCK_METHOD(void, stopScanning, (), (override));
    MOCK_METHOD(bool, isScanning, (), (const, override));
    MOCK_METHOD(std::vector<models::MediaFileModel>, scanDirectorySync, (const std::string& rootPath), (override));
    MOCK_METHOD(void, setProgressCallback, (services::ScanProgressCallback callback), (override));
    MOCK_METHOD(void, setCompleteCallback, (services::ScanCompleteCallback callback), (override));
    MOCK_METHOD(void, setMaxDepth, (int depth), (override));
    MOCK_METHOD(void, setFileExtensions, (const std::vector<std::string>& extensions), (override));
};

TEST(SourceControllerExtraTest, UsbInsertedCallbackBehaviour) {
    auto scanner = std::make_shared<MockScanner>();
    auto libRepo = std::make_shared<MockLibraryRepository>();
    auto libModel = std::make_shared<models::LibraryModel>();
    controllers::SourceController controller(scanner, libRepo, libModel);
    bool called = false;
    controller.setUsbInsertedCallback([&](const std::string& path){ called = true; });
    controller.handleUSBInserted("EVB-S32K144_DEVICE");
    EXPECT_FALSE(called);
    controller.handleUSBInserted("GenericUSB");
    EXPECT_TRUE(called);
}

TEST(SourceControllerExtraTest, OnScanCompleteUpdatesLibraryAndRepo) {
    auto scanner = std::make_shared<MockScanner>();
    services::ScanCompleteCallback completeCb;
    EXPECT_CALL(*scanner, setCompleteCallback(_)).WillOnce(testing::Invoke([&](auto f){ completeCb = f; }));
    auto libRepo = std::make_shared<MockLibraryRepository>();
    auto libModel = std::make_shared<models::LibraryModel>();
    EXPECT_CALL(*libRepo, clear()).Times(1);
    EXPECT_CALL(*libRepo, saveAll(_)).Times(1).WillOnce(testing::Return(true));
    controllers::SourceController controller(scanner, libRepo, libModel);
    ASSERT_TRUE(completeCb);
    std::vector<models::MediaFileModel> files{models::MediaFileModel("/tmp/a.mp3"), models::MediaFileModel("/tmp/b.mp3")};
    completeCb(files);
    EXPECT_EQ(libModel->getMediaCount(), 2u);
}

TEST(SourceControllerExtraTest, UsbInsertedStorageDeviceTriggersCallback) {
    auto scanner = std::make_shared<MockScanner>();
    auto libRepo = std::make_shared<MockLibraryRepository>();
    auto libModel = std::make_shared<models::LibraryModel>();
    controllers::SourceController controller(scanner, libRepo, libModel);
    std::string mount = "/tmp/usb_music_device";
    std::filesystem::create_directories(mount + "/Music");
    bool called = false;
    controller.setUsbInsertedCallback([&](const std::string& path){ called = true; });
    controller.handleUSBInserted(mount);
    EXPECT_TRUE(called);
    std::filesystem::remove_all(mount);
}

TEST(SourceControllerExtraTest, UsbInsertedS32KDeviceIgnored) {
    auto scanner = std::make_shared<MockScanner>();
    auto libRepo = std::make_shared<MockLibraryRepository>();
    auto libModel = std::make_shared<models::LibraryModel>();
    controllers::SourceController controller(scanner, libRepo, libModel);
    bool called = false;
    controller.setUsbInsertedCallback([&](const std::string& path){ called = true; });
    controller.handleUSBInserted("/media/EVB-S32K144-USB");
    EXPECT_FALSE(called);
}

TEST(SourceControllerExtraTest, HandleUSBRemovedClearsPath) {
    auto scanner = std::make_shared<MockScanner>();
    auto libRepo = std::make_shared<MockLibraryRepository>();
    auto libModel = std::make_shared<models::LibraryModel>();
    controllers::SourceController controller(scanner, libRepo, libModel);
    controller.selectDirectory("/tmp/path");
    controller.handleUSBRemoved();
    EXPECT_TRUE(controller.getCurrentSourcePath().empty());
}

TEST(SourceControllerExtraTest, ProgressCallbackForwarded) {
    auto scanner = std::make_shared<MockScanner>();
    services::ScanProgressCallback progressCb;
    EXPECT_CALL(*scanner, setProgressCallback(_)).WillOnce(testing::Invoke([&](auto f){ progressCb = f; }));
    auto libRepo = std::make_shared<MockLibraryRepository>();
    auto libModel = std::make_shared<models::LibraryModel>();
    controllers::SourceController controller(scanner, libRepo, libModel);
    int observed = 0;
    controller.setProgressCallback([&](int c, const std::string&){ observed = c; });
    ASSERT_TRUE(progressCb);
    progressCb(7, "/tmp");
    EXPECT_EQ(observed, 7);
}
