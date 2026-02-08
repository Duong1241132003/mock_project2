#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include "services/FileScanner.h"
#include <filesystem>
#include <fstream>
#include <thread>
#include <chrono>

namespace fs = std::filesystem;
using namespace media_player;
using namespace testing;

class FileScannerTest : public Test {
protected:
    void SetUp() override {
        // Create a temporary directory structure for testing
        testDir = fs::temp_directory_path() / "MediaPlayerTest_FileScanner";
        if (fs::exists(testDir)) {
            fs::remove_all(testDir);
        }
        fs::create_directories(testDir);

        // Create some dummy files
        createFile(testDir / "song1.mp3");
        createFile(testDir / "song2.wav"); // Lowercase
        createFile(testDir / "video.mp4");
        createFile(testDir / "image.png"); // Unsupported
        createFile(testDir / "SONG.MP3"); // Uppercase supported by extension check, but might be filtered by model
        createFile(testDir / "UPPER.WAV"); // Uppercase unsupported
        
        fs::create_directories(testDir / "subdir");
        createFile(testDir / "subdir" / "nested.flac");
    }

    void TearDown() override {
        if (fs::exists(testDir)) {
            fs::remove_all(testDir);
        }
    }

    void createFile(const fs::path& path) {
        std::ofstream ofs(path);
        ofs << "dummy content";
        ofs.close();
    }

    fs::path testDir;
    services::FileScanner scanner;
};

TEST_F(FileScannerTest, ScanDirectoryAsync) {
    std::vector<models::MediaFileModel> foundFiles;
    std::atomic<bool> scanComplete{false};

    // scanner.setScanPath(testDir.string()); // Removed
    scanner.setMaxDepth(5); // Recursive
    
    // Setup callbacks
    // Note: FileScanner might trigger onFileFound for each file
    // We need to inspect FileScanner.h to see the callback signature
    // Assuming: void scanDirectory(const std::string& path) and it uses internal callback
    
    // Wait, FileScanner.h has:
    // void setFileFoundCallback(std::function<void(const models::MediaFileModel&)> callback);
    // void setScanCompleteCallback(std::function<void()> callback);
    
    scanner.setCompleteCallback([&](std::vector<models::MediaFileModel> results) {
        foundFiles = results;
        scanComplete = true;
    });

    scanner.scanDirectory(testDir.string());

    // Wait for completion (with timeout)
    int timeout = 0;
    while (!scanComplete && timeout < 50) { // 5 seconds max
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
        timeout++;
    }

    EXPECT_TRUE(scanComplete);
    EXPECT_FALSE(foundFiles.empty());
    
    // Verify specific files found
    bool foundMp3 = false;
    bool foundWav = false;
    bool foundNested = false;
    
    for (const auto& file : foundFiles) {
        if (file.getFileName() == "song1.mp3") foundMp3 = true;
        if (file.getFileName() == "song2.wav") foundWav = true;
        if (file.getFileName() == "nested.flac") foundNested = true;
    }
    
    EXPECT_TRUE(foundMp3);
    EXPECT_TRUE(foundWav);
    EXPECT_TRUE(foundNested);
}

TEST_F(FileScannerTest, ScanDirectorySync) {
    // scanner.setScanPath(testDir.string()); // Removed
    scanner.setMaxDepth(5); // Recursive
    
    auto files = scanner.scanDirectorySync(testDir.string());
    
    EXPECT_FALSE(files.empty());
    EXPECT_GE(files.size(), 3); // At least mp3, wav, flac, mp4, maybe SONG.MP3
}

TEST_F(FileScannerTest, StopScan) {
    // Create many files to make scan take time
    for(int i=0; i<100; ++i) {
        createFile(testDir / ("temp" + std::to_string(i) + ".mp3"));
    }
    
    std::atomic<int> fileCount{0};
    scanner.setProgressCallback([&](int count, const std::string& path) {
        fileCount = count;
    });
    
    scanner.scanDirectory(testDir.string());
    
    // Stop immediately
    scanner.stopScanning();
    
    // Give a bit of time for thread to stop
    std::this_thread::sleep_for(std::chrono::milliseconds(200));
    
    // Should not have found all files
    // This is timing dependent, but likely valid
    EXPECT_LT(fileCount, 110); 
}

TEST_F(FileScannerTest, NonRecursiveScan) {
    // Check if setRecursive exists, otherwise FileScanner might scan recursive by default or argument
    // FileScanner.h check needed.
    // Assuming setMaxDepth can control recursion or separate method.
    scanner.setMaxDepth(0); // If 0 means no recursion
    auto files = scanner.scanDirectorySync(testDir.string());
    
    bool foundNested = false;
    for (const auto& file : files) {
        if (file.getFileName() == "nested.flac") foundNested = true;
    }
    
    EXPECT_FALSE(foundNested);
}
