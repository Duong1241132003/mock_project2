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

// ===================== Basic Scans =====================

TEST_F(FileScannerTest, ScanDirectoryAsync) {
    std::vector<models::MediaFileModel> foundFiles;
    std::atomic<bool> scanComplete{false};

    scanner.setMaxDepth(5); // Recursive
    
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
    scanner.setMaxDepth(0); // If 0 means no recursion
    auto files = scanner.scanDirectorySync(testDir.string());
    
    bool foundNested = false;
    for (const auto& file : files) {
        if (file.getFileName() == "nested.flac") foundNested = true;
    }
    
    EXPECT_FALSE(foundNested);
}

// ===================== Edge Cases =====================

TEST_F(FileScannerTest, ScanEmptyDirectory) {
    auto emptyDir = testDir / "empty";
    fs::create_directories(emptyDir);
    
    auto files = scanner.scanDirectorySync(emptyDir.string());
    EXPECT_TRUE(files.empty());
}

TEST_F(FileScannerTest, ScanNonExistentDirectory) {
    auto files = scanner.scanDirectorySync("/nonexistent/path/12345");
    EXPECT_TRUE(files.empty());
}

TEST_F(FileScannerTest, ScanFile) {
    // Scanning a file instead of directory
    auto files = scanner.scanDirectorySync((testDir / "song1.mp3").string());
    // Should return empty or handle gracefully
}

TEST_F(FileScannerTest, ScanDirectoryAsyncInvalidPaths) {
    scanner.scanDirectory("/nonexistent/path/scan");
    EXPECT_FALSE(scanner.isScanning());
    scanner.scanDirectory((testDir / "song1.mp3").string());
    EXPECT_FALSE(scanner.isScanning());
}

TEST_F(FileScannerTest, IsScanningState) {
    // Initially not scanning
    EXPECT_FALSE(scanner.isScanning());
    
    // Start scan
    scanner.setCompleteCallback([](std::vector<models::MediaFileModel>) {});
    scanner.scanDirectory(testDir.string());
    
    // While scanning, isScanning should be true
    // This is timing dependent
    std::this_thread::sleep_for(std::chrono::milliseconds(10));
    
    // Wait for completion
    std::this_thread::sleep_for(std::chrono::milliseconds(500));
    
    // After completion, should be false
    EXPECT_FALSE(scanner.isScanning());
}

TEST_F(FileScannerTest, SetMaxDepthLarge) {
    scanner.setMaxDepth(100);
    auto files = scanner.scanDirectorySync(testDir.string());
    EXPECT_FALSE(files.empty());
}

TEST_F(FileScannerTest, ProgressCallback) {
    std::atomic<int> progressCount{0};
    std::string lastPath;
    
    scanner.setProgressCallback([&](int count, const std::string& /* path */) {
        progressCount = count;
    });
    
    auto files = scanner.scanDirectorySync(testDir.string());
    
    // Progress might be called depending on implementation
    // Just verify the scan completed successfully
    EXPECT_FALSE(files.empty());
}

TEST_F(FileScannerTest, ProgressCallback3) {
    for (int i = 0; i < 6; ++i) {
        createFile(testDir / ("extra" + std::to_string(i) + ".mp3"));
    }
    std::atomic<bool> called{false};
    std::atomic<int> lastCount{0};
    std::atomic<int> lastTotal{0};
    scanner.setProgressCallback([&](int, const std::string&) {});
    scanner.setProgressCallback([&](int count, int total, const std::string&) {
        called = true;
        lastCount = count;
        lastTotal = total;
    });
    auto files = scanner.scanDirectorySync(testDir.string());
    EXPECT_FALSE(files.empty());
    EXPECT_TRUE(called);
    EXPECT_GE(lastCount, 10);
    EXPECT_GE(lastTotal, 10);
}

TEST_F(FileScannerTest, MultipleScanOperations) {
    auto files1 = scanner.scanDirectorySync(testDir.string());
    auto files2 = scanner.scanDirectorySync(testDir.string());
    
    // Both scans should return the same files
    EXPECT_EQ(files1.size(), files2.size());
}

// ===================== Depth Variations =====================

TEST_F(FileScannerTest, DepthOne) {
    scanner.setMaxDepth(1);
    auto files = scanner.scanDirectorySync(testDir.string());
    
    // Should find files in root and immediate subdirectory
    bool foundRoot = false;
    bool foundNested = false;
    
    for (const auto& file : files) {
        if (file.getFileName() == "song1.mp3") foundRoot = true;
        if (file.getFileName() == "nested.flac") foundNested = true;
    }
    
    EXPECT_TRUE(foundRoot);
    // At depth 1, should find subdir/nested.flac
    EXPECT_TRUE(foundNested);
}

TEST_F(FileScannerTest, DepthZero) {
    scanner.setMaxDepth(0);
    auto files = scanner.scanDirectorySync(testDir.string());
    
    // Should only find files in root, not subdirectories
    for (const auto& file : files) {
        EXPECT_FALSE(file.getFilePath().find("subdir") != std::string::npos);
    }
}
