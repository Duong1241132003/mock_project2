#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include "services/SerialCommunication.h"
#include <fcntl.h>
#include <unistd.h>
#include <termios.h>
#include <thread>
#include <chrono>

using namespace media_player::services;
using namespace testing;

class SerialCommunicationTest : public Test {
protected:
    void SetUp() override {
        // Create a pseudo-terminal pair for testing
        masterFd = posix_openpt(O_RDWR | O_NOCTTY);
        ASSERT_GE(masterFd, 0) << "Failed to open master PTY";
        
        ASSERT_EQ(grantpt(masterFd), 0) << "grantpt failed";
        ASSERT_EQ(unlockpt(masterFd), 0) << "unlockpt failed";
        
        char* slaveNamePtr = ptsname(masterFd);
        ASSERT_NE(slaveNamePtr, nullptr) << "ptsname failed";
        slaveName = std::string(slaveNamePtr);
    }
    
    void TearDown() override {
        if (masterFd >= 0) {
            close(masterFd);
        }
    }
    
    int masterFd = -1;
    std::string slaveName;
    SerialCommunication serial;
};

TEST_F(SerialCommunicationTest, OpenAndClose) {
    EXPECT_FALSE(serial.isOpen());
    
    // Open the slave end
    EXPECT_TRUE(serial.open(slaveName, 115200));
    EXPECT_TRUE(serial.isOpen());
    
    // Open again should return true
    EXPECT_TRUE(serial.open(slaveName, 115200));
    
    serial.close();
    EXPECT_FALSE(serial.isOpen());
}

TEST_F(SerialCommunicationTest, OpenInvalidPort) {
    EXPECT_FALSE(serial.open("/dev/nonexistent_port_12345", 115200));
    EXPECT_FALSE(serial.isOpen());
}

TEST_F(SerialCommunicationTest, SendData) {
    ASSERT_TRUE(serial.open(slaveName, 115200));
    
    std::string testData = "Hello World";
    EXPECT_TRUE(serial.sendData(testData));
    
    // Read from master to verify
    char buffer[256];
    ssize_t bytesRead = read(masterFd, buffer, sizeof(buffer) - 1);
    ASSERT_GT(bytesRead, 0);
    buffer[bytesRead] = '\0';
    
    EXPECT_EQ(std::string(buffer), testData);
}

TEST_F(SerialCommunicationTest, ReadDataCallback) {
    ASSERT_TRUE(serial.open(slaveName, 115200));
    
    std::string receivedData;
    std::atomic<bool> dataReceived{false};
    
    serial.setDataCallback([&](const std::string& data) {
        receivedData = data;
        dataReceived = true;
    });
    
    // Write to master
    std::string testData = "Response from device";
    write(masterFd, testData.c_str(), testData.length());
    
    // Wait for callback
    int timeout = 0;
    while (!dataReceived && timeout < 20) { // 2 seconds
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
        timeout++;
    }
    
    EXPECT_TRUE(dataReceived);
    EXPECT_EQ(receivedData, testData);
}

TEST_F(SerialCommunicationTest, ErrorCallback) {
    // To trigger an error, we might close the master FD while serial is open
    // This causes read() in the thread to fail (eventually)
    
    bool errorReceived = false;
    serial.setErrorCallback([&](const std::string& error) {
        errorReceived = true;
    });
    
    ASSERT_TRUE(serial.open(slaveName, 115200));
    
    // Close master to induce error on read
    close(masterFd);
    masterFd = -1;
    
    // Wait for read loop to detect error
    // Note: depending on OS, read might return 0 (EOF) instead of error
    // SerialCommunication::readThread handles >0 and <0. 
    // If read returns 0, it currently loops/sleeps in the code?
    // Let's check SerialCommunication.cpp:250 'bytesRead = read(...)'.
    // If read returns 0 (EOF), it does... nothing? 
    // It only handles >0 and <0.
    // If it returns 0, it just loops. So ErrorCallback might NOT be called on EOF.
    // We should check invalid port access for error callback.
    
    serial.close(); // Clean up first test attempt
    
    // Test invalid access permission or similiar? Hard with PTY.
    // Let's try opening a file that exists but is not a tty?
    // Or just check that error callback is set correctly.
    
    // Force call notifyError via a protected/friend method? No.
    // We can try to open a directory, which enters a failure path in open()
    
    serial.setErrorCallback([&](const std::string& error) {
        errorReceived = true;
    });
    
    EXPECT_FALSE(serial.open(".", 115200)); // Directories can't be opened as serial ports
    EXPECT_TRUE(errorReceived);
}
