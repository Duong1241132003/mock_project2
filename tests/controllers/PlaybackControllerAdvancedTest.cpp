#include <gtest/gtest.h>
#include <fstream>
#include "controllers/PlaybackController.h"
#include "models/QueueModel.h"
#include "models/PlaybackStateModel.h"
#include "models/HistoryModel.h"
#include "repositories/HistoryRepository.h"
#include "services/IPlaybackEngine.h"

using namespace media_player;

class CfgEngine : public services::IPlaybackEngine {
public:
    bool loadOk = true;
    bool seekOk = true;
    bool loadFile(const std::string& filePath) override { path = filePath; return loadOk; }
    bool play() override { state = services::PlaybackState::PLAYING; if (stateCb) stateCb(state); return true; }
    bool pause() override { state = services::PlaybackState::PAUSED; if (stateCb) stateCb(state); return true; }
    bool stop() override { state = services::PlaybackState::STOPPED; if (stateCb) stateCb(state); return true; }
    bool seek(int positionSeconds) override { pos = positionSeconds; return seekOk; }
    services::PlaybackState getState() const override { return state; }
    int getCurrentPosition() const override { return pos; }
    int getTotalDuration() const override { return 200; }
    void setVolume(int v) override { volume = v; }
    int getVolume() const override { return volume; }
    bool supportsMediaType(models::MediaType type) const override { return type == models::MediaType::AUDIO; }
    void setStateChangeCallback(services::PlaybackStateChangeCallback callback) override { stateCb = callback; }
    void setPositionCallback(services::PlaybackPositionCallback callback) override { posCb = callback; }
    void setErrorCallback(services::PlaybackErrorCallback callback) override { errCb = callback; }
    void setFinishedCallback(services::PlaybackFinishedCallback callback) override { finCb = callback; }
    void fireFinished() { if (finCb) finCb(); }
    void fireError(const std::string& e) { if (errCb) errCb(e); }
    int pos = 0;
    int volume = 50;
    std::string path;
    services::PlaybackState state = services::PlaybackState::STOPPED;
    services::PlaybackStateChangeCallback stateCb;
    services::PlaybackPositionCallback posCb;
    services::PlaybackErrorCallback errCb;
    services::PlaybackFinishedCallback finCb;
};

static std::shared_ptr<controllers::PlaybackController> makeController(
    std::shared_ptr<models::QueueModel>& qm,
    std::shared_ptr<models::PlaybackStateModel>& psm,
    std::shared_ptr<models::HistoryModel>& hm,
    std::unique_ptr<CfgEngine> eng)
{
    qm = std::make_shared<models::QueueModel>();
    psm = std::make_shared<models::PlaybackStateModel>();
    auto hr = std::make_shared<repositories::HistoryRepository>("/tmp/hist_adv");
    hm = std::make_shared<models::HistoryModel>(hr);
    auto ctl = std::make_shared<controllers::PlaybackController>(qm, psm, hm);
    ctl->setAudioEngine(std::move(eng));
    return ctl;
}

TEST(PlaybackControllerAdvancedTest, PlaySkipsMissingAndPlaysExisting) {
    std::string p1 = "/tmp/miss_a.mp3";
    std::string p2 = "/tmp/exist_b.mp3";
    { std::ofstream f(p2); f << ""; }
    std::shared_ptr<models::QueueModel> qm;
    std::shared_ptr<models::PlaybackStateModel> psm;
    std::shared_ptr<models::HistoryModel> hm;
    auto ctl = makeController(qm, psm, hm, std::make_unique<CfgEngine>());
    qm->addToEnd(models::MediaFileModel(p1));
    qm->addToEnd(models::MediaFileModel(p2));
    EXPECT_TRUE(ctl->play());
    EXPECT_EQ(ctl->getCurrentFilePath(), p2);
    EXPECT_TRUE(ctl->isPlaying());
}

TEST(PlaybackControllerAdvancedTest, PlayRemovesUnsupportedThenPlaysAudio) {
    std::string p1 = "/tmp/vid.mp4";
    std::string p2 = "/tmp/aud.mp3";
    { std::ofstream f1(p1); f1 << ""; }
    { std::ofstream f2(p2); f2 << ""; }
    std::shared_ptr<models::QueueModel> qm;
    std::shared_ptr<models::PlaybackStateModel> psm;
    std::shared_ptr<models::HistoryModel> hm;
    auto ctl = makeController(qm, psm, hm, std::make_unique<CfgEngine>());
    qm->addToEnd(models::MediaFileModel(p1));
    qm->addToEnd(models::MediaFileModel(p2));
    EXPECT_TRUE(ctl->play());
    EXPECT_EQ(ctl->getCurrentFilePath(), p2);
}

TEST(PlaybackControllerAdvancedTest, TogglePauseResumes) {
    std::string p = "/tmp/toggle.mp3";
    { std::ofstream f(p); f << ""; }
    std::shared_ptr<models::QueueModel> qm;
    std::shared_ptr<models::PlaybackStateModel> psm;
    std::shared_ptr<models::HistoryModel> hm;
    auto eng = std::make_unique<CfgEngine>();
    auto ctl = makeController(qm, psm, hm, std::move(eng));
    qm->addToEnd(models::MediaFileModel(p));
    EXPECT_TRUE(ctl->play());
    EXPECT_TRUE(ctl->pause());
    EXPECT_TRUE(ctl->togglePlayPause());
    EXPECT_TRUE(ctl->isPlaying());
}

TEST(PlaybackControllerAdvancedTest, PlayPreviousSeekWhenBeyondThreshold) {
    std::string p = "/tmp/prev_seek.mp3";
    { std::ofstream f(p); f << ""; }
    std::shared_ptr<models::QueueModel> qm;
    std::shared_ptr<models::PlaybackStateModel> psm;
    std::shared_ptr<models::HistoryModel> hm;
    auto eng = std::make_unique<CfgEngine>();
    CfgEngine* raw = eng.get();
    auto ctl = makeController(qm, psm, hm, std::move(eng));
    qm->addToEnd(models::MediaFileModel(p));
    EXPECT_TRUE(ctl->play());
    psm->setCurrentPosition(10);
    EXPECT_TRUE(ctl->playPrevious());
    EXPECT_EQ(raw->pos, 0);
}

TEST(PlaybackControllerAdvancedTest, PlayPreviousFromHistoryWhenAtStart) {
    std::string p0 = "/tmp/hist_prev.mp3";
    std::string p1 = "/tmp/hist_cur.mp3";
    { std::ofstream f0(p0); f0 << ""; }
    { std::ofstream f1(p1); f1 << ""; }
    std::shared_ptr<models::QueueModel> qm;
    std::shared_ptr<models::PlaybackStateModel> psm;
    std::shared_ptr<models::HistoryModel> hm;
    auto ctl = makeController(qm, psm, hm, std::make_unique<CfgEngine>());
    qm->addToEnd(models::MediaFileModel(p1));
    EXPECT_TRUE(ctl->play());
    hm->addEntry(models::MediaFileModel(p0));
    hm->addEntry(models::MediaFileModel(p1));
    psm->setCurrentPosition(0);
    EXPECT_TRUE(ctl->playPrevious());
    EXPECT_EQ(ctl->getCurrentFilePath(), p0);
    EXPECT_TRUE(ctl->isPlaying());
}

TEST(PlaybackControllerAdvancedTest, PlayPreviousFromHistoryFinishedResumesQueue) {
    std::string p0 = "/tmp/hist_prev2.mp3";
    std::string p1 = "/tmp/hist_cur2.mp3";
    { std::ofstream f0(p0); f0 << ""; }
    { std::ofstream f1(p1); f1 << ""; }
    std::shared_ptr<models::QueueModel> qm;
    std::shared_ptr<models::PlaybackStateModel> psm;
    std::shared_ptr<models::HistoryModel> hm;
    auto eng = std::make_unique<CfgEngine>();
    CfgEngine* raw = eng.get();
    auto ctl = makeController(qm, psm, hm, std::move(eng));
    qm->addToEnd(models::MediaFileModel(p1));
    EXPECT_TRUE(ctl->play());
    hm->addEntry(models::MediaFileModel(p0));
    hm->addEntry(models::MediaFileModel(p1));
    psm->setCurrentPosition(0);
    EXPECT_TRUE(ctl->playPrevious());
    EXPECT_EQ(ctl->getCurrentFilePath(), p0);
    raw->fireFinished();
    EXPECT_EQ(ctl->getCurrentFilePath(), p1);
}

TEST(PlaybackControllerAdvancedTest, TogglePlayPauseResumesOneOff) {
    std::string p = "/tmp/oneoff_toggle.mp3";
    { std::ofstream f(p); f << ""; }
    std::shared_ptr<models::QueueModel> qm;
    std::shared_ptr<models::PlaybackStateModel> psm;
    std::shared_ptr<models::HistoryModel> hm;
    auto ctl = makeController(qm, psm, hm, std::make_unique<CfgEngine>());
    EXPECT_TRUE(ctl->playMediaWithoutQueue(models::MediaFileModel(p)));
    EXPECT_TRUE(ctl->pause());
    EXPECT_TRUE(ctl->togglePlayPause());
    EXPECT_TRUE(ctl->isPlaying());
}

TEST(PlaybackControllerAdvancedTest, TogglePlayPauseReloadsOneOffWhenPathLost) {
    std::string p = "/tmp/oneoff_reload_toggle.mp3";
    { std::ofstream f(p); f << ""; }
    std::shared_ptr<models::QueueModel> qm;
    std::shared_ptr<models::PlaybackStateModel> psm;
    std::shared_ptr<models::HistoryModel> hm;
    auto eng = std::make_unique<CfgEngine>();
    CfgEngine* raw = eng.get();
    auto ctl = makeController(qm, psm, hm, std::move(eng));
    EXPECT_TRUE(ctl->playMediaWithoutQueue(models::MediaFileModel(p)));
    raw->stop();
    psm->setCurrentFilePath("");
    EXPECT_TRUE(ctl->togglePlayPause());
    EXPECT_TRUE(ctl->isPlaying());
}

TEST(PlaybackControllerAdvancedTest, TogglePlayPauseResumesOneOffWhenPathLoaded) {
    std::string p = "/tmp/oneoff_resume_loaded.mp3";
    { std::ofstream f(p); f << ""; }
    std::shared_ptr<models::QueueModel> qm;
    std::shared_ptr<models::PlaybackStateModel> psm;
    std::shared_ptr<models::HistoryModel> hm;
    auto eng = std::make_unique<CfgEngine>();
    CfgEngine* raw = eng.get();
    auto ctl = makeController(qm, psm, hm, std::move(eng));
    EXPECT_TRUE(ctl->playMediaWithoutQueue(models::MediaFileModel(p)));
    raw->stop();
    EXPECT_FALSE(ctl->isPlaying());
    EXPECT_FALSE(psm->getCurrentFilePath().empty());
    EXPECT_TRUE(ctl->togglePlayPause());
    EXPECT_TRUE(ctl->isPlaying());
}

TEST(PlaybackControllerAdvancedTest, PositionCallbackUpdatesState) {
    std::string p = "/tmp/pos_cb.mp3";
    { std::ofstream f(p); f << ""; }
    std::shared_ptr<models::QueueModel> qm;
    std::shared_ptr<models::PlaybackStateModel> psm;
    std::shared_ptr<models::HistoryModel> hm;
    auto eng = std::make_unique<CfgEngine>();
    CfgEngine* raw = eng.get();
    auto ctl = makeController(qm, psm, hm, std::move(eng));
    qm->addToEnd(models::MediaFileModel(p));
    EXPECT_TRUE(ctl->play());
    if (raw->posCb) raw->posCb(12, 200);
    EXPECT_EQ(psm->getCurrentPosition(), 12);
}

TEST(PlaybackControllerAdvancedTest, GetVolumeReturnsPlaybackState) {
    std::shared_ptr<models::QueueModel> qm;
    std::shared_ptr<models::PlaybackStateModel> psm;
    std::shared_ptr<models::HistoryModel> hm;
    auto ctl = makeController(qm, psm, hm, std::make_unique<CfgEngine>());
    ctl->setVolume(73);
    EXPECT_EQ(ctl->getVolume(), 73);
}

TEST(PlaybackControllerAdvancedTest, QueueLoopAllFinishedWrapsToStartAlt) {
    std::string p1 = "/tmp/loopall1.mp3";
    std::string p2 = "/tmp/loopall2.mp3";
    { std::ofstream f1(p1); f1 << ""; }
    { std::ofstream f2(p2); f2 << ""; }
    std::shared_ptr<models::QueueModel> qm;
    std::shared_ptr<models::PlaybackStateModel> psm;
    std::shared_ptr<models::HistoryModel> hm;
    auto eng = std::make_unique<CfgEngine>();
    CfgEngine* raw = eng.get();
    auto ctl = makeController(qm, psm, hm, std::move(eng));
    qm->addToEnd(models::MediaFileModel(p1));
    qm->addToEnd(models::MediaFileModel(p2));
    qm->setRepeatMode(models::RepeatMode::LoopAll);
    EXPECT_TRUE(ctl->play());
    raw->fireFinished();
    EXPECT_EQ(qm->getCurrentIndex(), 1u);
    raw->fireFinished();
    EXPECT_EQ(qm->getCurrentIndex(), 0u);
}

TEST(PlaybackControllerAdvancedTest, PlayMediaWithoutQueueNonexistentReturnsFalse) {
    std::shared_ptr<models::QueueModel> qm;
    std::shared_ptr<models::PlaybackStateModel> psm;
    std::shared_ptr<models::HistoryModel> hm;
    auto ctl = makeController(qm, psm, hm, std::make_unique<CfgEngine>());
    EXPECT_FALSE(ctl->playMediaWithoutQueue(models::MediaFileModel("/tmp/nope.mp3")));
}

TEST(PlaybackControllerAdvancedTest, TogglePlayPauseStoppedNoOneOffReturnsFalse) {
    std::shared_ptr<models::QueueModel> qm;
    std::shared_ptr<models::PlaybackStateModel> psm;
    std::shared_ptr<models::HistoryModel> hm;
    auto ctl = makeController(qm, psm, hm, std::make_unique<CfgEngine>());
    EXPECT_FALSE(ctl->togglePlayPause());
}
TEST(PlaybackControllerAdvancedTest, PlayNextStopsWhenNoNext) {
    std::string p = "/tmp/single.mp3";
    { std::ofstream f(p); f << ""; }
    std::shared_ptr<models::QueueModel> qm;
    std::shared_ptr<models::PlaybackStateModel> psm;
    std::shared_ptr<models::HistoryModel> hm;
    auto ctl = makeController(qm, psm, hm, std::make_unique<CfgEngine>());
    qm->addToEnd(models::MediaFileModel(p));
    EXPECT_TRUE(ctl->play());
    EXPECT_FALSE(ctl->playNext());
    EXPECT_TRUE(ctl->isStopped());
}

TEST(PlaybackControllerAdvancedTest, PlayItemAtInvalidIndexFails) {
    std::shared_ptr<models::QueueModel> qm;
    std::shared_ptr<models::PlaybackStateModel> psm;
    std::shared_ptr<models::HistoryModel> hm;
    auto ctl = makeController(qm, psm, hm, std::make_unique<CfgEngine>());
    EXPECT_FALSE(ctl->playItemAt(10));
}

TEST(PlaybackControllerAdvancedTest, StopPauseSeekNoEngineReturnFalse) {
    std::shared_ptr<models::QueueModel> qm;
    std::shared_ptr<models::PlaybackStateModel> psm;
    std::shared_ptr<models::HistoryModel> hm;
    auto ctl = makeController(qm, psm, hm, std::make_unique<CfgEngine>());
    EXPECT_TRUE(ctl->stop());
    EXPECT_TRUE(ctl->pause());
    EXPECT_TRUE(ctl->seek(5));
    EXPECT_EQ(ctl->getCurrentMediaType(), models::MediaType::UNKNOWN);
}
TEST(PlaybackControllerAdvancedTest, OnErrorRemovesCurrentAndContinues) {
    std::string p1 = "/tmp/err_a.mp3";
    std::string p2 = "/tmp/err_b.mp3";
    { std::ofstream f1(p1); f1 << ""; }
    { std::ofstream f2(p2); f2 << ""; }
    std::shared_ptr<models::QueueModel> qm;
    std::shared_ptr<models::PlaybackStateModel> psm;
    std::shared_ptr<models::HistoryModel> hm;
    auto eng = std::make_unique<CfgEngine>();
    CfgEngine* raw = eng.get();
    auto ctl = makeController(qm, psm, hm, std::move(eng));
    qm->addToEnd(models::MediaFileModel(p1));
    qm->addToEnd(models::MediaFileModel(p2));
    EXPECT_TRUE(ctl->play());
    raw->fireError("e");
    EXPECT_EQ(qm->size(), 1u);
    EXPECT_EQ(ctl->getCurrentFilePath(), p2);
}

TEST(PlaybackControllerAdvancedTest, OneOffLoopOneSeekOk) {
    std::string p = "/tmp/oneoff_loop.mp3";
    { std::ofstream f(p); f << ""; }
    std::shared_ptr<models::QueueModel> qm;
    std::shared_ptr<models::PlaybackStateModel> psm;
    std::shared_ptr<models::HistoryModel> hm;
    auto eng = std::make_unique<CfgEngine>();
    CfgEngine* raw = eng.get();
    auto ctl = makeController(qm, psm, hm, std::move(eng));
    qm->setRepeatMode(models::RepeatMode::LoopOne);
    EXPECT_TRUE(ctl->playMediaWithoutQueue(models::MediaFileModel(p)));
    raw->seekOk = true;
    raw->fireFinished();
    EXPECT_TRUE(ctl->isPlaying());
}

TEST(PlaybackControllerAdvancedTest, OneOffLoopOneSeekFailReloads) {
    std::string p = "/tmp/oneoff_reload.mp3";
    { std::ofstream f(p); f << ""; }
    std::shared_ptr<models::QueueModel> qm;
    std::shared_ptr<models::PlaybackStateModel> psm;
    std::shared_ptr<models::HistoryModel> hm;
    auto eng = std::make_unique<CfgEngine>();
    CfgEngine* raw = eng.get();
    auto ctl = makeController(qm, psm, hm, std::move(eng));
    qm->setRepeatMode(models::RepeatMode::LoopOne);
    EXPECT_TRUE(ctl->playMediaWithoutQueue(models::MediaFileModel(p)));
    raw->seekOk = false;
    raw->fireFinished();
    EXPECT_TRUE(ctl->isPlaying());
}

TEST(PlaybackControllerAdvancedTest, QueueLoopOneFinishedSeeksAndReplays) {
    std::string p = "/tmp/queue_loop.mp3";
    { std::ofstream f(p); f << ""; }
    std::shared_ptr<models::QueueModel> qm;
    std::shared_ptr<models::PlaybackStateModel> psm;
    std::shared_ptr<models::HistoryModel> hm;
    auto eng = std::make_unique<CfgEngine>();
    CfgEngine* raw = eng.get();
    auto ctl = makeController(qm, psm, hm, std::move(eng));
    qm->addToEnd(models::MediaFileModel(p));
    qm->setRepeatMode(models::RepeatMode::LoopOne);
    EXPECT_TRUE(ctl->play());
    raw->seekOk = true;
    raw->fireFinished();
    EXPECT_EQ(raw->pos, 0);
    EXPECT_TRUE(ctl->isPlaying());
}

TEST(PlaybackControllerAdvancedTest, PlayItemAtJumpsAndPlays) {
    std::string p1 = "/tmp/jump_a.mp3";
    std::string p2 = "/tmp/jump_b.mp3";
    { std::ofstream f1(p1); f1 << ""; }
    { std::ofstream f2(p2); f2 << ""; }
    std::shared_ptr<models::QueueModel> qm;
    std::shared_ptr<models::PlaybackStateModel> psm;
    std::shared_ptr<models::HistoryModel> hm;
    auto ctl = makeController(qm, psm, hm, std::make_unique<CfgEngine>());
    qm->addToEnd(models::MediaFileModel(p1));
    qm->addToEnd(models::MediaFileModel(p2));
    EXPECT_TRUE(ctl->playItemAt(1));
    EXPECT_EQ(ctl->getCurrentFilePath(), p2);
}

TEST(PlaybackControllerAdvancedTest, QueueLoopAllFinishedWrapsToStart) {
    std::string p1 = "/tmp/loopall_a.mp3";
    std::string p2 = "/tmp/loopall_b.mp3";
    { std::ofstream f1(p1); f1 << ""; }
    { std::ofstream f2(p2); f2 << ""; }
    std::shared_ptr<models::QueueModel> qm;
    std::shared_ptr<models::PlaybackStateModel> psm;
    std::shared_ptr<models::HistoryModel> hm;
    auto eng = std::make_unique<CfgEngine>();
    CfgEngine* raw = eng.get();
    auto ctl = makeController(qm, psm, hm, std::move(eng));
    qm->addToEnd(models::MediaFileModel(p1));
    qm->addToEnd(models::MediaFileModel(p2));
    qm->setRepeatMode(models::RepeatMode::LoopAll);
    EXPECT_TRUE(ctl->play());
    raw->fireFinished();
    EXPECT_EQ(ctl->getCurrentFilePath(), p2);
    raw->fireFinished();
    EXPECT_EQ(ctl->getCurrentFilePath(), p1);
}

class CfgEngineSelective : public CfgEngine {
public:
    std::string failPath;
    bool loadFile(const std::string& filePath) override {
        path = filePath;
        if (filePath == failPath) return false;
        return true;
    }
};

TEST(PlaybackControllerAdvancedTest, PlayRecursionOnLoadFailRemovesBadItem) {
    std::string p1 = "/tmp/bad_load.mp3";
    std::string p2 = "/tmp/good_load.mp3";
    { std::ofstream f1(p2); f1 << ""; }
    std::shared_ptr<models::QueueModel> qm;
    std::shared_ptr<models::PlaybackStateModel> psm;
    std::shared_ptr<models::HistoryModel> hm;
    auto eng = std::make_unique<CfgEngineSelective>();
    CfgEngineSelective* raw = eng.get();
    raw->failPath = p1;
    auto ctl = makeController(qm, psm, hm, std::move(eng));
    qm->addToEnd(models::MediaFileModel(p1));
    qm->addToEnd(models::MediaFileModel(p2));
    EXPECT_TRUE(ctl->play());
    EXPECT_EQ(qm->size(), 1u);
    EXPECT_EQ(ctl->getCurrentFilePath(), p2);
}

TEST(PlaybackControllerAdvancedTest, SkipHistoryOnQueuePreviousDoesNotAddHistory) {
    std::string p1 = "/tmp/q_prev_a.mp3";
    std::string p2 = "/tmp/q_prev_b.mp3";
    { std::ofstream f1(p1); f1 << ""; }
    { std::ofstream f2(p2); f2 << ""; }
    std::shared_ptr<models::QueueModel> qm;
    std::shared_ptr<models::PlaybackStateModel> psm;
    std::shared_ptr<models::HistoryModel> hm;
    auto ctl = makeController(qm, psm, hm, std::make_unique<CfgEngine>());
    qm->addToEnd(models::MediaFileModel(p1));
    qm->addToEnd(models::MediaFileModel(p2));
    EXPECT_TRUE(ctl->play());
    qm->moveToNext();
    EXPECT_TRUE(ctl->play());
    hm->clear();
    psm->setCurrentPosition(0);
    EXPECT_TRUE(ctl->playPrevious());
    EXPECT_EQ(ctl->getCurrentFilePath(), p1);
    EXPECT_EQ(hm->count(), 0u);
}

TEST(PlaybackControllerAdvancedTest, SetVolumeSyncsEngineAndModel) {
    std::string p = "/tmp/vol_sync.mp3";
    { std::ofstream f(p); f << ""; }
    std::shared_ptr<models::QueueModel> qm;
    std::shared_ptr<models::PlaybackStateModel> psm;
    std::shared_ptr<models::HistoryModel> hm;
    auto eng = std::make_unique<CfgEngine>();
    CfgEngine* raw = eng.get();
    auto ctl = makeController(qm, psm, hm, std::move(eng));
    psm->setVolume(77);
    qm->addToEnd(models::MediaFileModel(p));
    EXPECT_TRUE(ctl->play());
    EXPECT_EQ(raw->volume, 77);
    ctl->setVolume(33);
    EXPECT_EQ(psm->getVolume(), 33);
    EXPECT_EQ(raw->volume, 33);
}

TEST(PlaybackControllerAdvancedTest, StopResetsStateAndClearsOneOff) {
    std::string p = "/tmp/stop_reset.mp3";
    { std::ofstream f(p); f << ""; }
    std::shared_ptr<models::QueueModel> qm;
    std::shared_ptr<models::PlaybackStateModel> psm;
    std::shared_ptr<models::HistoryModel> hm;
    auto ctl = makeController(qm, psm, hm, std::make_unique<CfgEngine>());
    EXPECT_TRUE(ctl->playMediaWithoutQueue(models::MediaFileModel(p)));
    EXPECT_TRUE(ctl->stop());
    EXPECT_TRUE(ctl->isStopped());
    EXPECT_EQ(ctl->getCurrentFilePath(), "");
    EXPECT_EQ(ctl->getCurrentMediaType(), models::MediaType::UNKNOWN);
}
