#ifndef MOCK_PLAYBACK_ENGINE_H
#define MOCK_PLAYBACK_ENGINE_H

#include "services/IPlaybackEngine.h"
#include <gmock/gmock.h>

namespace media_player {
namespace test {

class MockPlaybackEngine : public services::IPlaybackEngine {
public:
    MOCK_METHOD(bool, loadFile, (const std::string&), (override));
    MOCK_METHOD(bool, play, (), (override));
    MOCK_METHOD(bool, pause, (), (override));
    MOCK_METHOD(bool, stop, (), (override));
    MOCK_METHOD(bool, seek, (int), (override));
    MOCK_METHOD(void, releaseResources, (), (override));
    MOCK_METHOD(services::PlaybackState, getState, (), (const, override));
    MOCK_METHOD(int, getCurrentPosition, (), (const, override));
    MOCK_METHOD(int, getTotalDuration, (), (const, override));
    MOCK_METHOD(void, setVolume, (int), (override));
    MOCK_METHOD(int, getVolume, (), (const, override));
    MOCK_METHOD(bool, supportsMediaType, (models::MediaType), (const, override));
    
    // Callbacks - store them to simulate events
    MOCK_METHOD(void, setStateChangeCallback, (services::PlaybackStateChangeCallback), (override));
    MOCK_METHOD(void, setPositionCallback, (services::PlaybackPositionCallback), (override));
    MOCK_METHOD(void, setErrorCallback, (services::PlaybackErrorCallback), (override));
    MOCK_METHOD(void, setFinishedCallback, (services::PlaybackFinishedCallback), (override));
    
    // Helpers to trigger callbacks
    void triggerStateChange(services::PlaybackState state) {
        if (stateCallback) stateCallback(state);
    }
    
    void triggerPosition(int current, int total) {
        if (posCallback) posCallback(current, total);
    }
    
    void triggerError(const std::string& error) {
        if (errCallback) errCallback(error);
    }
    
    void triggerFinished() {
        if (finishCallback) finishCallback();
    }
    
    // Capture callbacks
    void DelegateToFake() {
        ON_CALL(*this, setStateChangeCallback(testing::_)).WillByDefault(testing::Invoke([this](services::PlaybackStateChangeCallback cb){ stateCallback = cb; }));
        ON_CALL(*this, setPositionCallback(testing::_)).WillByDefault(testing::Invoke([this](services::PlaybackPositionCallback cb){ posCallback = cb; }));
        ON_CALL(*this, setErrorCallback(testing::_)).WillByDefault(testing::Invoke([this](services::PlaybackErrorCallback cb){ errCallback = cb; }));
        ON_CALL(*this, setFinishedCallback(testing::_)).WillByDefault(testing::Invoke([this](services::PlaybackFinishedCallback cb){ finishCallback = cb; }));
    }

private:
    services::PlaybackStateChangeCallback stateCallback;
    services::PlaybackPositionCallback posCallback;
    services::PlaybackErrorCallback errCallback;
    services::PlaybackFinishedCallback finishCallback;
};

} // namespace test
} // namespace media_player

#endif // MOCK_PLAYBACK_ENGINE_H
