#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include "controllers/SourceController.h"
#include "mocks/MockFileScanner.h"
#include "mocks/MockLibraryRepository.h"
#include "models/LibraryModel.h" 

using namespace media_player;
using namespace testing;

class SourceControllerTest : public Test {
protected:
    void SetUp() override {
        mockScanner = std::make_shared<test::MockFileScanner>();
        mockRepo = std::make_shared<test::MockLibraryRepository>();
        
        // Use real LibraryModel as it's a simple container and methods are non-virtual
        realModel = std::make_shared<models::LibraryModel>();
        
        // Pass all dependencies
        controller = std::make_shared<controllers::SourceController>(mockScanner, mockRepo, realModel);
    }
    
    void TearDown() override {
        // Ensure threads stop if needed
    }

    std::shared_ptr<test::MockFileScanner> mockScanner;
    std::shared_ptr<test::MockLibraryRepository> mockRepo;
    std::shared_ptr<models::LibraryModel> realModel;
    std::shared_ptr<controllers::SourceController> controller;
};

TEST_F(SourceControllerTest, InitialState) {
    // Should be empty or default
    EXPECT_TRUE(controller->getCurrentSourcePath().empty()); 
}

TEST_F(SourceControllerTest, SelectDirectory) {
    // Select directory just sets the path
    controller->selectDirectory("/media/usb/drive");
    EXPECT_EQ(controller->getCurrentSourcePath(), "/media/usb/drive");

    // Scan triggers the scanner interactions
    // Expect scanner to be configured (if implemented in scanCurrentDirectory or before)
    // SourceController implementation of scanCurrentDirectory:
    // LOG_INFO("Starting scan of: " + m_currentSourcePath);
    // m_fileScanner->scanDirectory(m_currentSourcePath);

    EXPECT_CALL(*mockScanner, scanDirectory("/media/usb/drive")).Times(1);
    
    // Note: setFileExtensions and setMaxDepth might NOT be called in scanCurrentDirectory
    // They might be called in constructor or not at all if defaults used.
    // Checking SourceController.cpp: it does NOT call setMaxDepth or setFileExtensions in scanCurrentDirectory.
    // So remove those expectations.
    
    controller->scanCurrentDirectory();
}

TEST_F(SourceControllerTest, StopScan) {
    EXPECT_CALL(*mockScanner, stopScanning()).Times(AtLeast(1));
    controller->stopScan();
}

TEST_F(SourceControllerTest, IsScanning) {
    EXPECT_CALL(*mockScanner, isScanning()).WillOnce(Return(true));
    EXPECT_TRUE(controller->isScanning());
}

#include <filesystem>
#include <fstream>

class SourceControllerUSBTest : public SourceControllerTest {
protected:
    std::filesystem::path tempDir;

    void SetUp() override {
        SourceControllerTest::SetUp();
        tempDir = std::filesystem::temp_directory_path() / "MediaPlayerUSBTest";
        std::filesystem::create_directories(tempDir);
        
        // Configure controller to monitor this temp dir
        controller->setMediaRoot(tempDir.string());
        
        // Ensure monitoring is started for these tests if relevant, 
        // though handleUSBInserted is tested directly here.
        // For integration tests of the loop, we would call startMonitoring()
    }

    void TearDown() override {
        SourceControllerTest::TearDown();
        std::filesystem::remove_all(tempDir);
    }
};

TEST_F(SourceControllerUSBTest, HandleUSBInserted_StorageDevice_FallbackScan) {
    // Setup: Create a "Music" directory to make it look like storage
    std::filesystem::create_directories(tempDir / "Music");
    
    // Expect fallback behavior: selects directory and scans
    EXPECT_CALL(*mockScanner, scanDirectory(tempDir.string())).Times(1);
    
    controller->handleUSBInserted(tempDir.string());
    
    EXPECT_EQ(controller->getCurrentSourcePath(), tempDir.string());
}

TEST_F(SourceControllerUSBTest, HandleUSBInserted_StorageDevice_WithCallback) {
    // Setup
    std::filesystem::create_directories(tempDir / "Videos");
    
    bool callbackCalled = false;
    std::string callbackPath;
    
    controller->setUsbInsertedCallback([&](const std::string& path) {
        callbackCalled = true;
        callbackPath = path;
    });
    
    // Expect NO scan call because callback handles it
    EXPECT_CALL(*mockScanner, scanDirectory(_)).Times(0);
    
    controller->handleUSBInserted(tempDir.string());
    
    EXPECT_TRUE(callbackCalled);
    EXPECT_EQ(callbackPath, tempDir.string());
}

TEST_F(SourceControllerUSBTest, HandleUSBInserted_S32K144_Ignored) {
    // Setup: Name must contain "EVB-S32K144"
    std::filesystem::path s32kPath = tempDir / "EVB-S32K144_Mounted";
    std::filesystem::create_directories(s32kPath);
    
    // Even if it has music, it should be ignored as storage if it's S32K144
    std::filesystem::create_directories(s32kPath / "Music");
    
    EXPECT_CALL(*mockScanner, scanDirectory(_)).Times(0);
    
    bool callbackCalled = false;
    controller->setUsbInsertedCallback([&](const std::string&) { callbackCalled = true; });
    
    controller->handleUSBInserted(s32kPath.string());
    
    EXPECT_FALSE(callbackCalled);
}

TEST_F(SourceControllerUSBTest, HandleUSBInserted_Empty_DefaultsToStorage) {
    // Setup: Empty directory, no media folders/files
    // Implementation defaults to treating it as storage
    
    EXPECT_CALL(*mockScanner, scanDirectory(tempDir.string())).Times(1);
    
    controller->handleUSBInserted(tempDir.string());
}

TEST_F(SourceControllerUSBTest, HandleUSBInserted_MediaFilesRoot_Detected) {
    // Setup: Add a .mp3 file to root
    std::ofstream(tempDir / "song.mp3");
    
    EXPECT_CALL(*mockScanner, scanDirectory(tempDir.string())).Times(1);
    
    controller->handleUSBInserted(tempDir.string());
}

TEST_F(SourceControllerUSBTest, MonitorLoop_DetectsNewDevice) {
    // This test verifies the thread actually picks up changes
    
    std::atomic<bool> detected{false};
    controller->setUsbInsertedCallback([&](const std::string& path) {
        if (path.find("NewDrive") != std::string::npos) {
            detected = true;
        }
    });
    
    controller->startMonitoring();
    
    // Give thread time to start and do initial scan
    std::this_thread::sleep_for(std::chrono::milliseconds(2500)); 
    
    // Create new directory
    std::filesystem::create_directories(tempDir / "NewDrive" / "Music");
    
    // Wait for detection loop
    int retries = 0;
    while (!detected && retries < 40) { // Increased retries just in case
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
        retries++;
    }
    
    controller->stopScan(); 
    
    EXPECT_TRUE(detected);
}

TEST_F(SourceControllerUSBTest, Callbacks_AreForwarded) {
    // Local mocks for this specific test to capture constructor calls
    auto testScanner = std::make_shared<test::MockFileScanner>();
    auto testRepo = std::make_shared<test::MockLibraryRepository>();
    auto testModel = std::make_shared<models::LibraryModel>();
    
    // Variables to capture the internal callbacks passed to scanner
    media_player::services::ScanProgressCallback internalProgress;
    media_player::services::ScanCompleteCallback internalComplete;
    
    // Setup expectations to capture the callbacks during controller construction
    EXPECT_CALL(*testScanner, setProgressCallback(_))
        .WillOnce(Invoke([&](media_player::services::ScanProgressCallback cb) {
            internalProgress = cb;
        }));
        
    EXPECT_CALL(*testScanner, setCompleteCallback(_))
        .WillOnce(Invoke([&](media_player::services::ScanCompleteCallback cb) {
            internalComplete = cb;
        }));
        
    // Create controller - this should trigger the EXPECT_CALLs
    auto testController = std::make_shared<controllers::SourceController>(testScanner, testRepo, testModel);
    
    // Now setup external callbacks on the controller
    bool externalProgressCalled = false;
    testController->setProgressCallback([&](int, const std::string&) { externalProgressCalled = true; });
    
    bool externalCompleteCalled = false;
    testController->setCompleteCallback([&](int) { externalCompleteCalled = true; });
    
    // Trigger the internal callbacks (simulate scanner updates)
    if (internalProgress) internalProgress(1, "test");
    if (internalComplete) internalComplete(std::vector<models::MediaFileModel>{});
    
    // Verify external callbacks were invoked
    EXPECT_TRUE(externalProgressCalled);
    EXPECT_TRUE(externalCompleteCalled);
}

TEST_F(SourceControllerUSBTest, MonitorLoop_Removal) {
    std::atomic<bool> removed{false};
    // No explicit callback for removal in SourceController public API (handleUSBRemoved is public but no callback setter?)
    // Wait, SourceController.h has handleUSBRemoved() but no setUsbRemovedCallback.
    // It calls stopScan() and clears path.
    // The monitor loop calls knownMounts.erase(it).
    // It commented out // handleUSBRemoved(); // Optional
    
    // So actually, the removal logic in MonitorLoop DOES NOTHING visible externally except updating internal state.
    // Validation: We can check if it detects re-insertion?
    
    // 1. Insert
    std::filesystem::create_directories(tempDir / "RemoveTest");
    controller->startMonitoring();
    std::this_thread::sleep_for(std::chrono::milliseconds(1500));
    
    // 2. Remove
    std::filesystem::remove_all(tempDir / "RemoveTest");
    std::this_thread::sleep_for(std::chrono::milliseconds(1500));
    // Internal state should update.
    
    // 3. Re-insert
    bool reInserted = false;
    controller->setUsbInsertedCallback([&](const std::string&) { reInserted = true; });
    std::filesystem::create_directories(tempDir / "RemoveTest");
    
    int retries = 0;
    while (!reInserted && retries < 40) {
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
        retries++;
    }
    
    controller->stopScan();
    EXPECT_TRUE(reInserted);
}
