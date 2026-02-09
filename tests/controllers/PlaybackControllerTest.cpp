#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include "controllers/PlaybackController.h"
#include "models/QueueModel.h"
#include "models/PlaybackStateModel.h"
#include "repositories/HistoryRepository.h"
#include "../mocks/MockPlaybackEngine.h"
#include <filesystem>
#include <fstream>

namespace fs = std::filesystem;
using namespace media_player;
using namespace testing;

class PlaybackControllerTest : public Test {
protected:
    void SetUp() override {
        // Create dummy files directory
        fs::create_directories("/tmp/test_media");
        
        queueModel = std::make_shared<models::QueueModel>();
        playbackState = std::make_shared<models::PlaybackStateModel>();
        
        std::string tempHistoryPath = "./test_data/history_" + std::to_string(std::rand());
        fs::create_directories(tempHistoryPath);
        historyRepo = std::make_shared<repositories::HistoryRepository>(tempHistoryPath);
        
        controller = std::make_shared<controllers::PlaybackController>(
            queueModel,
            playbackState,
            historyRepo
        );
        
        // Setup mock engine
        auto mockEngine = std::make_unique<test::MockPlaybackEngine>();
        mockEnginePtr = mockEngine.get();
        
        // Setup default behaviors - nice mock
        ON_CALL(*mockEnginePtr, supportsMediaType(_)).WillByDefault(Return(true));
        ON_CALL(*mockEnginePtr, getState()).WillByDefault(Return(services::PlaybackState::STOPPED));
        ON_CALL(*mockEnginePtr, getCurrentPosition()).WillByDefault(Return(0));
        ON_CALL(*mockEnginePtr, getTotalDuration()).WillByDefault(Return(100));
        
        
        mockEnginePtr->DelegateToFake();
        
        controller->setAudioEngine(std::move(mockEngine));
    }
    
    void TearDown() override {
        std::filesystem::remove_all("./test_data");
        std::filesystem::remove_all("/tmp/test_media");
    }

    std::shared_ptr<models::QueueModel> queueModel;
    std::shared_ptr<models::PlaybackStateModel> playbackState;
    std::shared_ptr<repositories::HistoryRepository> historyRepo;
    std::shared_ptr<controllers::PlaybackController> controller;
    test::MockPlaybackEngine* mockEnginePtr;
    
    std::string createDummyFile(const std::string& name) {
        std::string path = "/tmp/test_media/" + name;
        std::ofstream(path).close();
        return path;
    }
};


TEST_F(PlaybackControllerTest, Play_Successfully) {
    std::string file = createDummyFile("test.mp3");
    models::MediaFileModel media(file);
    queueModel->addToEnd(media);
    
    EXPECT_CALL(*mockEnginePtr, loadFile(file)).WillOnce(Return(true));
    EXPECT_CALL(*mockEnginePtr, play())
        .WillOnce(DoAll(
            InvokeWithoutArgs([this](){ mockEnginePtr->triggerStateChange(services::PlaybackState::PLAYING); }),
            Return(true)
        ));
    
    EXPECT_TRUE(controller->play());
    EXPECT_TRUE(controller->isPlaying());
}

TEST_F(PlaybackControllerTest, Pause_Successfully) {
    // Simulate playing state
    playbackState->setState(models::PlaybackState::PLAYING);
    
    EXPECT_CALL(*mockEnginePtr, pause())
        .WillOnce(DoAll(
            InvokeWithoutArgs([this](){ mockEnginePtr->triggerStateChange(services::PlaybackState::PAUSED); }),
            Return(true)
        ));
    EXPECT_TRUE(controller->pause());
    EXPECT_TRUE(controller->isPaused());
}

TEST_F(PlaybackControllerTest, Stop_Successfully) {
    // stop() can be called by destructor too
    EXPECT_CALL(*mockEnginePtr, stop())
        .WillOnce(DoAll(
             InvokeWithoutArgs([this](){ mockEnginePtr->triggerStateChange(services::PlaybackState::STOPPED); }),
             Return(true)
        ))
        .WillRepeatedly(Return(true));
        
    EXPECT_TRUE(controller->stop());
    EXPECT_EQ(playbackState->getCurrentFilePath(), "");
}

TEST_F(PlaybackControllerTest, Seek_Successfully) {
    EXPECT_CALL(*mockEnginePtr, seek(10)).WillOnce(Return(true));
    EXPECT_TRUE(controller->seek(10));
}

TEST_F(PlaybackControllerTest, VolumeControl) {
    EXPECT_CALL(*mockEnginePtr, setVolume(50));
    controller->setVolume(50);
    EXPECT_EQ(playbackState->getVolume(), 50);
}

TEST_F(PlaybackControllerTest, PlayNext_MovesQueue) {
    std::string file1 = createDummyFile("test1.mp3");
    std::string file2 = createDummyFile("test2.mp3");
    
    queueModel->addToEnd(models::MediaFileModel(file1));
    queueModel->addToEnd(models::MediaFileModel(file2));
    
    // First Play
    EXPECT_CALL(*mockEnginePtr, loadFile(file1)).WillOnce(Return(true));
    EXPECT_CALL(*mockEnginePtr, play()).WillOnce(Return(true));
    controller->play();
    
    // Play Next
    EXPECT_CALL(*mockEnginePtr, stop()).Times(AtLeast(0)); 
    EXPECT_CALL(*mockEnginePtr, loadFile(file2)).WillOnce(Return(true));
    EXPECT_CALL(*mockEnginePtr, play()).WillOnce(Return(true));
    
    EXPECT_TRUE(controller->playNext());
    EXPECT_EQ(queueModel->getCurrentIndex(), 1u);
}


TEST_F(PlaybackControllerTest, PlayPrevious_Rewind) {
    std::string file1 = createDummyFile("test1.mp3");
    queueModel->addToEnd(models::MediaFileModel(file1));
    
    // Fake state: playing, position > 2 sec
    EXPECT_CALL(*mockEnginePtr, loadFile(file1)).WillOnce(Return(true));
    EXPECT_CALL(*mockEnginePtr, play()).WillOnce(Return(true));
    controller->play();
    
    ON_CALL(*mockEnginePtr, getCurrentPosition()).WillByDefault(Return(5));
    // Also we need to ensure PlaybackController updates its internal state from engine
    // PlaybackController usually has a loop or callback to update state model.
    // For unit test without event loop, we might need to manually update state model
    // OR mock the engine to return 5 when asked.
    
    // PlaybackController::playPrevious() checks m_playbackStateModel->getCurrentPosition() ?
    // Or m_currentEngine->getCurrentPosition()?
    // Let's assume it checks the model if updated via callbacks, or engine directly.
    // If logic: if (pos > 2) seek(0).
    
    // We need to force logic.
    // If controller reads from model, we must update model.
    playbackState->setCurrentPosition(5); 
    playbackState->setState(models::PlaybackState::PLAYING);
    
    EXPECT_CALL(*mockEnginePtr, seek(0)).WillOnce(Return(true));
    
    EXPECT_TRUE(controller->playPrevious());
}

TEST_F(PlaybackControllerTest, PlayMediaWithoutQueue) {
    std::string file = createDummyFile("oneoff.mp3");
    models::MediaFileModel media(file);
    
    EXPECT_CALL(*mockEnginePtr, loadFile(file)).WillOnce(Return(true));
    EXPECT_CALL(*mockEnginePtr, play()).WillOnce(Return(true));
    
    EXPECT_TRUE(controller->playMediaWithoutQueue(media));
    EXPECT_EQ(playbackState->getCurrentFilePath(), file);
}

TEST_F(PlaybackControllerTest, TogglePlayPause) {
    // 1. Not playing, queue empty -> return false
    EXPECT_FALSE(controller->togglePlayPause());
    
    // 2. Queue has item
    std::string file = createDummyFile("toggle.mp3");
    queueModel->addToEnd(models::MediaFileModel(file));
    
    EXPECT_CALL(*mockEnginePtr, loadFile(file)).WillOnce(Return(true));
    EXPECT_CALL(*mockEnginePtr, play()).WillOnce(Return(true));
    EXPECT_TRUE(controller->togglePlayPause());
    
    // 3. Playing -> Pause
    playbackState->setState(models::PlaybackState::PLAYING);
    EXPECT_CALL(*mockEnginePtr, pause()).WillOnce(Return(true));
    EXPECT_TRUE(controller->togglePlayPause());
}
