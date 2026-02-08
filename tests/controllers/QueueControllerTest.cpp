#include <gtest/gtest.h>
#include "controllers/QueueController.h"
#include "models/QueueModel.h"
#include "models/PlaylistModel.h"
#include "models/MediaFileModel.h"

using namespace media_player;

class QueueControllerTest : public testing::Test {
protected:
    void SetUp() override {
        queueModel = std::make_shared<models::QueueModel>();
        controller = std::make_shared<controllers::QueueController>(queueModel);
    }
    std::shared_ptr<models::QueueModel> queueModel;
    std::shared_ptr<controllers::QueueController> controller;
};

TEST_F(QueueControllerTest, AddAndRemoveItems) {
    models::MediaFileModel a("/tmp/a.mp3");
    models::MediaFileModel b("/tmp/b.mp3");
    controller->addToQueue(a);
    controller->addToQueue(b);
    EXPECT_EQ(queueModel->size(), 2u);
    EXPECT_TRUE(controller->removeFromQueue(0));
    EXPECT_EQ(queueModel->size(), 1u);
    EXPECT_FALSE(controller->removeFromQueue(5));
}

TEST_F(QueueControllerTest, AddNextAndJumpMove) {
    models::MediaFileModel a("/tmp/a.mp3");
    models::MediaFileModel b("/tmp/b.mp3");
    models::MediaFileModel c("/tmp/c.mp3");
    controller->addToQueue(a);
    controller->addToQueue(b);
    controller->addToQueueNext(c);
    EXPECT_EQ(queueModel->size(), 3u);
    EXPECT_TRUE(controller->jumpToIndex(1));
    EXPECT_TRUE(controller->moveItem(1, 2));
}

TEST_F(QueueControllerTest, PlaylistAndMultipleAdd) {
    models::PlaylistModel pl("p");
    pl.addItem(models::MediaFileModel("/tmp/x.mp3"));
    pl.addItem(models::MediaFileModel("/tmp/y.mp3"));
    controller->addPlaylistToQueue(pl);
    std::vector<models::MediaFileModel> extra{models::MediaFileModel("/tmp/z.mp3")};
    controller->addMultipleToQueue(extra);
    EXPECT_EQ(queueModel->size(), 3u);
}

TEST_F(QueueControllerTest, NavigationAndClear) {
    controller->addToQueue(models::MediaFileModel("/tmp/a.mp3"));
    controller->addToQueue(models::MediaFileModel("/tmp/b.mp3"));
    EXPECT_TRUE(controller->moveToNext());
    EXPECT_TRUE(controller->moveToPrevious());
    controller->clearQueue();
}

TEST_F(QueueControllerTest, ShuffleAndRepeatModes) {
    EXPECT_FALSE(controller->isShuffleEnabled());
    controller->toggleShuffle();
    EXPECT_TRUE(controller->isShuffleEnabled());
    EXPECT_FALSE(controller->isRepeatEnabled());
    controller->cycleRepeatMode();
    EXPECT_TRUE(controller->isRepeatEnabled());
}

TEST_F(QueueControllerTest, AddPlaylistAndMultiple) {
    models::PlaylistModel pl("pl");
    pl.addItem(models::MediaFileModel("/tmp/a.mp3"));
    pl.addItem(models::MediaFileModel("/tmp/b.mp3"));
    controller->addPlaylistToQueue(pl);
    EXPECT_EQ(queueModel->size(), 2u);
    std::vector<models::MediaFileModel> more{models::MediaFileModel("/tmp/c.mp3"), models::MediaFileModel("/tmp/d.mp3")};
    controller->addMultipleToQueue(more);
    EXPECT_EQ(queueModel->size(), 4u);
}

TEST_F(QueueControllerTest, RemoveByPathAndMoveItem) {
    auto a = models::MediaFileModel("/tmp/a.mp3");
    auto b = models::MediaFileModel("/tmp/b.mp3");
    auto c = models::MediaFileModel("/tmp/c.mp3");
    controller->addToQueue(a);
    controller->addToQueue(b);
    controller->addToQueue(c);
    EXPECT_TRUE(controller->removeByPath("/tmp/b.mp3"));
    EXPECT_EQ(queueModel->size(), 2u);
    EXPECT_TRUE(controller->moveItem(1, 0));
    auto items = controller->getAllItems();
    EXPECT_EQ(items[0].getFilePath(), "/tmp/c.mp3");
}

TEST_F(QueueControllerTest, JumpToIndexSetsCurrent) {
    controller->addToQueue(models::MediaFileModel("/tmp/a.mp3"));
    controller->addToQueue(models::MediaFileModel("/tmp/b.mp3"));
    EXPECT_TRUE(controller->jumpToIndex(1));
    EXPECT_EQ(controller->getCurrentIndex(), 1u);
}

TEST_F(QueueControllerTest, CycleRepeatModeTransitions) {
    EXPECT_FALSE(controller->isRepeatEnabled());
    controller->cycleRepeatMode(); // None -> LoopOne
    EXPECT_TRUE(controller->isRepeatEnabled());
    controller->cycleRepeatMode(); // LoopOne -> LoopAll
    EXPECT_TRUE(controller->isRepeatEnabled());
    controller->cycleRepeatMode(); // LoopAll -> None
    EXPECT_FALSE(controller->isRepeatEnabled());
}

TEST_F(QueueControllerTest, PlaybackOrderReflectsShuffle) {
    controller->addToQueue(models::MediaFileModel("/tmp/a.mp3"));
    controller->addToQueue(models::MediaFileModel("/tmp/b.mp3"));
    controller->setShuffle(true);
    auto order = controller->getPlaybackOrderItems();
    EXPECT_EQ(order.size(), queueModel->size());
}

TEST_F(QueueControllerTest, ApiSurfaceCoverageAndNegativeBranches) {
    EXPECT_TRUE(controller->isEmpty());
    EXPECT_EQ(controller->getQueueSize(), 0u);
    EXPECT_FALSE(controller->removeFromQueue(0));
    EXPECT_FALSE(controller->removeByPath("/tmp/none.mp3"));
    EXPECT_FALSE(controller->jumpToIndex(1));
    EXPECT_FALSE(controller->moveToNext());
    EXPECT_FALSE(controller->moveToPrevious());
    EXPECT_FALSE(controller->moveItem(0, 1));
    EXPECT_EQ(controller->getRepeatMode(), models::RepeatMode::None);
    controller->cycleRepeatMode();
    controller->setRepeat(models::RepeatMode::LoopAll);
    EXPECT_TRUE(controller->isRepeatEnabled());
    EXPECT_FALSE(controller->isShuffleEnabled());
    controller->toggleShuffle();
    EXPECT_TRUE(controller->isShuffleEnabled());
    controller->setShuffle(false);
    EXPECT_FALSE(controller->isShuffleEnabled());
    controller->addMultipleToQueue({ models::MediaFileModel("/tmp/x.mp3"), models::MediaFileModel("/tmp/y.mp3") });
    EXPECT_FALSE(controller->isEmpty());
    auto items = controller->getAllItems();
    auto playOrder = controller->getPlaybackOrderItems();
    auto current = controller->getCurrentItem();
    EXPECT_EQ(items.size(), controller->getQueueSize());
    EXPECT_EQ(playOrder.size(), items.size());
    EXPECT_TRUE(current.has_value());
    EXPECT_EQ(controller->getCurrentIndex(), 0u);
}
