#include <gtest/gtest.h>
#include <fstream>
#include "controllers/PlaybackController.h"
#include "models/QueueModel.h"
#include "models/PlaybackStateModel.h"
#include "models/HistoryModel.h"
#include "repositories/HistoryRepository.h"
#include "services/IPlaybackEngine.h"

using namespace media_player;

class FakeEngine : public services::IPlaybackEngine {
public:
    bool loadFile(const std::string& filePath) override { path = filePath; return true; }
    bool play() override { state = services::PlaybackState::PLAYING; if (stateCb) stateCb(state); return true; }
    bool pause() override { state = services::PlaybackState::PAUSED; if (stateCb) stateCb(state); return true; }
    bool stop() override { state = services::PlaybackState::STOPPED; if (stateCb) stateCb(state); return true; }
    bool seek(int positionSeconds) override { pos = positionSeconds; return true; }
    services::PlaybackState getState() const override { return state; }
    int getCurrentPosition() const override { return pos; }
    int getTotalDuration() const override { return 120; }
    void setVolume(int v) override { volume = v; }
    int getVolume() const override { return volume; }
    bool supportsMediaType(models::MediaType type) const override { return type == models::MediaType::AUDIO; }
    void setStateChangeCallback(services::PlaybackStateChangeCallback callback) override { stateCb = callback; }
    void setPositionCallback(services::PlaybackPositionCallback callback) override { posCb = callback; }
    void setErrorCallback(services::PlaybackErrorCallback callback) override { errCb = callback; }
    void setFinishedCallback(services::PlaybackFinishedCallback callback) override { finCb = callback; }
    void triggerFinished() { if (finCb) finCb(); }
private:
    std::string path;
    int pos = 0;
    int volume = 50;
    services::PlaybackState state = services::PlaybackState::STOPPED;
    services::PlaybackStateChangeCallback stateCb;
    services::PlaybackPositionCallback posCb;
    services::PlaybackErrorCallback errCb;
    services::PlaybackFinishedCallback finCb;
};

TEST(PlaybackControllerEngineTest, PlayAndTogglePauseWithFakeEngine) {
    std::string path = "/tmp/test_audio.mp3";
    { std::ofstream f(path); f << ""; }
    auto queueModel = std::make_shared<models::QueueModel>();
    queueModel->addToEnd(models::MediaFileModel(path));
    auto playbackState = std::make_shared<models::PlaybackStateModel>();
    auto historyRepo = std::make_shared<repositories::HistoryRepository>("/tmp/hist");
    auto historyModel = std::make_shared<models::HistoryModel>(historyRepo);
    auto controller = std::make_shared<controllers::PlaybackController>(queueModel, playbackState, historyModel);
    auto fake = std::make_unique<FakeEngine>();
    controller->setAudioEngine(std::move(fake));
    EXPECT_TRUE(controller->play());
    EXPECT_TRUE(controller->isPlaying());
    EXPECT_TRUE(controller->togglePlayPause());
    EXPECT_TRUE(controller->isPaused());
}

TEST(PlaybackControllerEngineTest, PlayNextAtEndStopsPlayback) {
    std::string path = "/tmp/test_end.mp3";
    { std::ofstream f(path); f << ""; }
    auto queueModel = std::make_shared<models::QueueModel>();
    queueModel->addToEnd(models::MediaFileModel(path));
    auto playbackState = std::make_shared<models::PlaybackStateModel>();
    auto historyRepo = std::make_shared<repositories::HistoryRepository>("/tmp/hist2");
    auto historyModel = std::make_shared<models::HistoryModel>(historyRepo);
    auto controller = std::make_shared<controllers::PlaybackController>(queueModel, playbackState, historyModel);
    auto fake = std::make_unique<FakeEngine>();
    controller->setAudioEngine(std::move(fake));
    EXPECT_TRUE(controller->play());
    EXPECT_TRUE(controller->isPlaying());
    EXPECT_FALSE(controller->playNext());
    EXPECT_TRUE(controller->isStopped());
}

TEST(PlaybackControllerEngineTest, PlayPreviousRewindsWhenPlayingBeyondThreshold) {
    std::string path = "/tmp/test_prev.mp3";
    { std::ofstream f(path); f << ""; }
    auto queueModel = std::make_shared<models::QueueModel>();
    queueModel->addToEnd(models::MediaFileModel(path));
    auto playbackState = std::make_shared<models::PlaybackStateModel>();
    auto historyRepo = std::make_shared<repositories::HistoryRepository>("/tmp/hist3");
    auto historyModel = std::make_shared<models::HistoryModel>(historyRepo);
    auto controller = std::make_shared<controllers::PlaybackController>(queueModel, playbackState, historyModel);
    auto fake = std::make_unique<FakeEngine>();
    controller->setAudioEngine(std::move(fake));
    EXPECT_TRUE(controller->play());
    playbackState->setCurrentPosition(5);
    EXPECT_TRUE(controller->playPrevious());
}
