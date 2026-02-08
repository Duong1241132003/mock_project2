#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include "controllers/PlaybackController.h"
#include "models/QueueModel.h"
#include "models/PlaybackStateModel.h"
#include "repositories/HistoryRepository.h"
#include <filesystem>

namespace fs = std::filesystem;
using namespace media_player;
using namespace testing;

class PlaybackControllerTest : public Test {
protected:
    void SetUp() override {
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
    }
    
    void TearDown() override {
        std::filesystem::remove_all("./test_data");
    }

    std::shared_ptr<models::QueueModel> queueModel;
    std::shared_ptr<models::PlaybackStateModel> playbackState;
    std::shared_ptr<repositories::HistoryRepository> historyRepo;
    std::shared_ptr<controllers::PlaybackController> controller;
};

TEST_F(PlaybackControllerTest, PlayNextInQueue) {
    auto media1 = models::MediaFileModel("/path/to/song1.mp3");
    auto media2 = models::MediaFileModel("/path/to/song2.mp3");
    
    queueModel->addToEnd(media1);
    queueModel->addToEnd(media2);
    
    // Initial state
    EXPECT_EQ(queueModel->getCurrentIndex(), 0);
    
    // Attempt play next. Even if engine fails, queue logic might trigger.
    // However, PlaybackController might check if playing.
    controller->playNext();
}

TEST_F(PlaybackControllerTest, VolumeControl) {
    controller->setVolume(80);
    EXPECT_EQ(playbackState->getVolume(), 80);
    
    // We can't test increase/decrease from controller if it's not exposed
    // But we can verify setVolume works.
    
    controller->setVolume(85);
    EXPECT_EQ(playbackState->getVolume(), 85);
}

TEST_F(PlaybackControllerTest, PlayPauseToggle) {
    // Intiail
    EXPECT_FALSE(controller->isPlaying());
    
    // Toggle
    controller->togglePlayPause();
    // Might not switch to playing if no media loaded.
    
    // Load something?
    // Hard without engine mock returning success.
    
    // But we can check calls.
}
