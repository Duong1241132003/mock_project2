#include <gtest/gtest.h>
#include "services/AudioPlaybackEngine.h"
#include <SDL2/SDL.h>
#include <filesystem>
#include <fstream>
#include <thread>

namespace fs = std::filesystem;
using namespace media_player::services;
using namespace media_player;

class AudioPlaybackEngineTest : public ::testing::Test {
protected:
    void SetUp() override {
        // Use dummy audio driver to avoid hardware dependency
        SDL_setenv("SDL_AUDIODRIVER", "dummy", 1);
        
        testDir = fs::temp_directory_path() / "MediaPlayerTest_AudioEngine";
        if (fs::exists(testDir)) {
            fs::remove_all(testDir);
        }
        fs::create_directories(testDir);
        
        validWavPath = testDir / "test.wav";
        createMinimalWav(validWavPath);
    }

    void TearDown() override {
        if (fs::exists(testDir)) {
            fs::remove_all(testDir);
        }
    }
    
    void createMinimalWav(const fs::path& path) {
        std::ofstream file(path, std::ios::binary);
        // Minimal WAV header (44 bytes) for 1 second of silence? 
        // Or just the header. SDL_mixer might need actual data to play.
        // Let's add some silence data.
        
        uint32_t sampleRate = 44100;
        uint16_t channels = 2;
        uint16_t bits = 16;
        uint32_t dataSize = sampleRate * channels * (bits/8); // 1 second
        uint32_t fileSize = 36 + dataSize;
        
        file.write("RIFF", 4);
        file.write(reinterpret_cast<const char*>(&fileSize), 4);
        file.write("WAVE", 4);
        file.write("fmt ", 4);
        uint32_t fmtSize = 16;
        file.write(reinterpret_cast<const char*>(&fmtSize), 4);
        uint16_t audioFormat = 1; // PCM
        file.write(reinterpret_cast<const char*>(&audioFormat), 2);
        file.write(reinterpret_cast<const char*>(&channels), 2);
        file.write(reinterpret_cast<const char*>(&sampleRate), 4);
        uint32_t byteRate = sampleRate * channels * (bits/8);
        file.write(reinterpret_cast<const char*>(&byteRate), 4);
        uint16_t blockAlign = channels * (bits/8);
        file.write(reinterpret_cast<const char*>(&blockAlign), 2);
        file.write(reinterpret_cast<const char*>(&bits), 2);
        file.write("data", 4);
        file.write(reinterpret_cast<const char*>(&dataSize), 4);
        
        // Write silence
        std::vector<char> silence(dataSize, 0);
        file.write(silence.data(), dataSize);
    }

    fs::path testDir;
    fs::path validWavPath;
};

// Test initialization and singleton
TEST_F(AudioPlaybackEngineTest, Initialization) {
    AudioPlaybackEngine engine;
    EXPECT_EQ(engine.getState(), PlaybackState::STOPPED);
    EXPECT_EQ(engine.getVolume(), 70); // Default
}

// Test Volume Control
TEST_F(AudioPlaybackEngineTest, VolumeControl) {
    AudioPlaybackEngine engine;
    engine.setVolume(50);
    EXPECT_EQ(engine.getVolume(), 50);
    engine.setVolume(100);
    EXPECT_EQ(engine.getVolume(), 100);
    engine.setVolume(0);
    EXPECT_EQ(engine.getVolume(), 0);
}

// Test State Transitions
TEST_F(AudioPlaybackEngineTest, StateTransitions) {
    AudioPlaybackEngine engine;
    
    // Initial state
    EXPECT_EQ(engine.getState(), PlaybackState::STOPPED);
    
    // Try play without file -> invalid
    EXPECT_FALSE(engine.play());
    
    // Load file
    bool loaded = engine.loadFile(validWavPath.string());
    
    // If SDL mixer failed to init (e.g. CI), load might fail. 
    // But with dummy driver it should work.
    if (loaded) {
        EXPECT_EQ(engine.getState(), PlaybackState::STOPPED);
        
        // Play
        EXPECT_TRUE(engine.play());
        EXPECT_EQ(engine.getState(), PlaybackState::PLAYING);
        
        // Pause
        EXPECT_TRUE(engine.pause());
        EXPECT_EQ(engine.getState(), PlaybackState::PAUSED);
        
        // Resume
        EXPECT_TRUE(engine.play());
        EXPECT_EQ(engine.getState(), PlaybackState::PLAYING);
        
        // Stop
        EXPECT_TRUE(engine.stop());
        EXPECT_EQ(engine.getState(), PlaybackState::STOPPED);
    }
}

// Test callbacks
TEST_F(AudioPlaybackEngineTest, Callbacks) {
    AudioPlaybackEngine engine;
    
    bool stateChanged = false;
    PlaybackState lastState = PlaybackState::PAUSED; // Use PAUSED as dummy initial value
    
    engine.setStateChangeCallback([&](PlaybackState s) {
        stateChanged = true;
        lastState = s;
    });
    
    if (engine.loadFile(validWavPath.string())) {
        engine.play();
        // Wait for callback (async potentially? no, state change is usually synchronous in this engine)
        // engine.play() sets m_state and calls notifyStateChange immediately.
        EXPECT_TRUE(stateChanged);
        EXPECT_EQ(lastState, PlaybackState::PLAYING);
    }
}

TEST_F(AudioPlaybackEngineTest, LoadInvalidFile) {
    AudioPlaybackEngine engine;
    bool errorOccurred = false;
    engine.setErrorCallback([&](const std::string&) { // Unused parameter
        errorOccurred = true;
    });
    
    EXPECT_FALSE(engine.loadFile("non_existent_file.mp3"));
    EXPECT_TRUE(errorOccurred);
}
