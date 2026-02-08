#include <gtest/gtest.h>
#include <fstream>
#include "controllers/PlaybackController.h"
#include "models/QueueModel.h"
#include "models/PlaybackStateModel.h"
#include "repositories/HistoryRepository.h"
#include "services/IPlaybackEngine.h"

using namespace media_player;

class NavFakeEngine : public services::IPlaybackEngine {
public:
    bool loadFile(const std::string& filePath) override { path = filePath; return true; }
    bool play() override { state = services::PlaybackState::PLAYING; if (stateCb) stateCb(state); return true; }
    bool pause() override { state = services::PlaybackState::PAUSED; if (stateCb) stateCb(state); return true; }
    bool stop() override { state = services::PlaybackState::STOPPED; if (stateCb) stateCb(state); return true; }
    bool seek(int positionSeconds) override { pos = positionSeconds; if (posCb) posCb(pos, 180); return true; }
    services::PlaybackState getState() const override { return state; }
    int getCurrentPosition() const override { return pos; }
    int getTotalDuration() const override { return 180; }
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

TEST(PlaybackControllerNavigationTest, NextPreviousStopSeek) {
    std::string p1 = "/tmp/nav_a.mp3";
    std::string p2 = "/tmp/nav_b.mp3";
    { std::ofstream f1(p1); f1 << ""; }
    { std::ofstream f2(p2); f2 << ""; }
    auto queueModel = std::make_shared<models::QueueModel>();
    queueModel->addToEnd(models::MediaFileModel(p1));
    queueModel->addToEnd(models::MediaFileModel(p2));
    auto playbackState = std::make_shared<models::PlaybackStateModel>();
    auto historyRepo = std::make_shared<repositories::HistoryRepository>("/tmp/hist_nav");
    controllers::PlaybackController controller(queueModel, playbackState, historyRepo);
    auto fake = std::make_unique<NavFakeEngine>();
    controller.setAudioEngine(std::move(fake));
    EXPECT_TRUE(controller.play());
    EXPECT_TRUE(controller.playNext());
    EXPECT_TRUE(controller.playPrevious());
    EXPECT_TRUE(controller.seek(10));
    EXPECT_TRUE(controller.stop());
}

TEST(PlaybackControllerNavigationTest, PlayWithoutQueueAndFinish) {
    std::string p = "/tmp/oneoff.mp3";
    { std::ofstream f(p); f << ""; }
    auto queueModel = std::make_shared<models::QueueModel>();
    auto playbackState = std::make_shared<models::PlaybackStateModel>();
    auto historyRepo = std::make_shared<repositories::HistoryRepository>("/tmp/hist_oneoff");
    controllers::PlaybackController controller(queueModel, playbackState, historyRepo);
    auto fake = std::make_unique<NavFakeEngine>();
    NavFakeEngine* raw = fake.get();
    controller.setAudioEngine(std::move(fake));
    controller.playMediaWithoutQueue(models::MediaFileModel(p));
    raw->triggerFinished();
    EXPECT_FALSE(controller.isPlaying());
}
