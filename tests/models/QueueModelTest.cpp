#include <gtest/gtest.h>
#include "models/QueueModel.h"
#include "models/MediaFileModel.h"

using namespace media_player::models;

class QueueModelTest : public ::testing::Test {
protected:
    QueueModel model;
};

// ===================== Cơ bản =====================

TEST_F(QueueModelTest, InitialState) {
    EXPECT_TRUE(model.getAllItems().empty());
    EXPECT_EQ(model.getCurrentIndex(), 0);
    EXPECT_TRUE(model.isEmpty());
    EXPECT_EQ(model.size(), 0);
    EXPECT_FALSE(model.isShuffleEnabled());
    EXPECT_EQ(model.getRepeatMode(), RepeatMode::None);
}

TEST_F(QueueModelTest, AddToQueue) {
    MediaFileModel file1("/1.mp3");
    model.addToEnd(file1);
    
    EXPECT_EQ(model.getAllItems().size(), 1);
    EXPECT_FALSE(model.isEmpty());
}

TEST_F(QueueModelTest, Clear) {
    MediaFileModel file1("/1.mp3");
    model.addToEnd(file1);
    model.clear();
    EXPECT_TRUE(model.getAllItems().empty());
    EXPECT_EQ(model.getCurrentIndex(), 0);
}

// ===================== addNext =====================

TEST_F(QueueModelTest, AddNextEmpty) {
    // Khi queue rỗng, addNext thêm vào cuối
    MediaFileModel file1("/1.mp3");
    model.addNext(file1);
    EXPECT_EQ(model.size(), 1);
    auto item = model.getItemAt(0);
    ASSERT_TRUE(item.has_value());
    EXPECT_EQ(item->getFilePath(), "/1.mp3");
}

TEST_F(QueueModelTest, AddNextAfterCurrent) {
    // Thêm item ngay sau current index
    MediaFileModel file1("/1.mp3");
    MediaFileModel file2("/2.mp3");
    MediaFileModel file3("/3.mp3");
    model.addToEnd(file1);
    model.addToEnd(file2);
    model.jumpTo(0); // current = 0
    model.addNext(file3); // Thêm vào index 1
    
    EXPECT_EQ(model.size(), 3);
    auto item = model.getItemAt(1);
    ASSERT_TRUE(item.has_value());
    EXPECT_EQ(item->getFilePath(), "/3.mp3");
}

// ===================== addAt =====================

TEST_F(QueueModelTest, AddAtValidPosition) {
    MediaFileModel file1("/1.mp3");
    MediaFileModel file2("/2.mp3");
    MediaFileModel file3("/3.mp3");
    model.addToEnd(file1);
    model.addToEnd(file2);
    model.addAt(file3, 1);
    
    EXPECT_EQ(model.size(), 3);
    auto item = model.getItemAt(1);
    ASSERT_TRUE(item.has_value());
    EXPECT_EQ(item->getFilePath(), "/3.mp3");
}

TEST_F(QueueModelTest, AddAtOutOfBounds) {
    // Khi position >= size, thêm vào cuối
    MediaFileModel file1("/1.mp3");
    MediaFileModel file2("/2.mp3");
    model.addToEnd(file1);
    model.addAt(file2, 100);
    
    EXPECT_EQ(model.size(), 2);
    auto item = model.getItemAt(1);
    ASSERT_TRUE(item.has_value());
    EXPECT_EQ(item->getFilePath(), "/2.mp3");
}

// ===================== removeByPath =====================

TEST_F(QueueModelTest, RemoveByPathSuccess) {
    MediaFileModel file1("/1.mp3");
    MediaFileModel file2("/2.mp3");
    model.addToEnd(file1);
    model.addToEnd(file2);
    
    EXPECT_TRUE(model.removeByPath("/1.mp3"));
    EXPECT_EQ(model.size(), 1);
    auto item = model.getItemAt(0);
    ASSERT_TRUE(item.has_value());
    EXPECT_EQ(item->getFilePath(), "/2.mp3");
}

TEST_F(QueueModelTest, RemoveByPathNotFound) {
    MediaFileModel file1("/1.mp3");
    model.addToEnd(file1);
    
    EXPECT_FALSE(model.removeByPath("/notexist.mp3"));
    EXPECT_EQ(model.size(), 1);
}

// ===================== Navigation =====================

TEST_F(QueueModelTest, Navigation) {
    MediaFileModel file1("/1.mp3");
    MediaFileModel file2("/2.mp3");
    model.addToEnd(file1);
    model.addToEnd(file2);
    
    model.jumpTo(0);
    EXPECT_EQ(model.getCurrentIndex(), 0);
    auto current = model.getCurrentItem();
    EXPECT_TRUE(current.has_value());
    EXPECT_EQ(current->getFilePath(), "/1.mp3");
    
    bool hasNext = model.hasNext();
    EXPECT_TRUE(hasNext);
    if(hasNext) {
        model.moveToNext();
        EXPECT_EQ(model.getCurrentIndex(), 1);
    }
    
    bool hasPrev = model.hasPrevious();
    EXPECT_TRUE(hasPrev);
    if(hasPrev) {
        model.moveToPrevious();
        EXPECT_EQ(model.getCurrentIndex(), 0);
    }
}

TEST_F(QueueModelTest, RemoveAtIndex) {
    MediaFileModel file1("/1.mp3");
    MediaFileModel file2("/2.mp3");
    MediaFileModel file3("/3.mp3");
    model.addToEnd(file1);
    model.addToEnd(file2);
    model.addToEnd(file3);
    
    model.jumpTo(1);
    model.removeAt(0);
    
    EXPECT_EQ(model.getAllItems().size(), 2);
    EXPECT_EQ(model.getCurrentIndex(), 0);
    
    auto current = model.getCurrentItem();
    ASSERT_TRUE(current.has_value());
    EXPECT_EQ(current->getFilePath(), "/2.mp3");
}

// ===================== getNextItem / getPreviousItem =====================

TEST_F(QueueModelTest, GetNextItemNormal) {
    MediaFileModel file1("/1.mp3");
    MediaFileModel file2("/2.mp3");
    model.addToEnd(file1);
    model.addToEnd(file2);
    model.jumpTo(0);
    
    auto next = model.getNextItem();
    ASSERT_TRUE(next.has_value());
    EXPECT_EQ(next->getFilePath(), "/2.mp3");
}

TEST_F(QueueModelTest, GetNextItemEmpty) {
    auto next = model.getNextItem();
    EXPECT_FALSE(next.has_value());
}

TEST_F(QueueModelTest, GetNextItemAtEnd) {
    MediaFileModel file1("/1.mp3");
    model.addToEnd(file1);
    model.jumpTo(0);
    
    auto next = model.getNextItem();
    EXPECT_FALSE(next.has_value());
}

TEST_F(QueueModelTest, GetPreviousItemNormal) {
    MediaFileModel file1("/1.mp3");
    MediaFileModel file2("/2.mp3");
    model.addToEnd(file1);
    model.addToEnd(file2);
    model.jumpTo(1);
    
    auto prev = model.getPreviousItem();
    ASSERT_TRUE(prev.has_value());
    EXPECT_EQ(prev->getFilePath(), "/1.mp3");
}

TEST_F(QueueModelTest, GetPreviousItemAtStart) {
    MediaFileModel file1("/1.mp3");
    model.addToEnd(file1);
    model.jumpTo(0);
    
    auto prev = model.getPreviousItem();
    EXPECT_FALSE(prev.has_value());
}

TEST_F(QueueModelTest, GetPreviousItemEmpty) {
    auto prev = model.getPreviousItem();
    EXPECT_FALSE(prev.has_value());
}

// ===================== getItemAt =====================

TEST_F(QueueModelTest, GetItemAtValid) {
    MediaFileModel file1("/1.mp3");
    MediaFileModel file2("/2.mp3");
    model.addToEnd(file1);
    model.addToEnd(file2);
    
    auto item = model.getItemAt(1);
    ASSERT_TRUE(item.has_value());
    EXPECT_EQ(item->getFilePath(), "/2.mp3");
}

TEST_F(QueueModelTest, GetItemAtOutOfBounds) {
    MediaFileModel file1("/1.mp3");
    model.addToEnd(file1);
    
    auto item = model.getItemAt(100);
    EXPECT_FALSE(item.has_value());
}

// ===================== getCurrentItem =====================

TEST_F(QueueModelTest, GetCurrentItemEmpty) {
    auto current = model.getCurrentItem();
    EXPECT_FALSE(current.has_value());
}

// ===================== moveItem =====================

TEST_F(QueueModelTest, MoveItemSuccess) {
    MediaFileModel file1("/1.mp3");
    MediaFileModel file2("/2.mp3");
    MediaFileModel file3("/3.mp3");
    model.addToEnd(file1);
    model.addToEnd(file2);
    model.addToEnd(file3);
    
    // Di chuyển item từ index 0 -> index 2
    EXPECT_TRUE(model.moveItem(0, 2));
    
    auto item0 = model.getItemAt(0);
    auto item2 = model.getItemAt(2);
    ASSERT_TRUE(item0.has_value());
    ASSERT_TRUE(item2.has_value());
    EXPECT_EQ(item0->getFilePath(), "/2.mp3");
    EXPECT_EQ(item2->getFilePath(), "/1.mp3");
}

TEST_F(QueueModelTest, MoveItemOutOfBounds) {
    MediaFileModel file1("/1.mp3");
    model.addToEnd(file1);
    
    EXPECT_FALSE(model.moveItem(0, 100));
    EXPECT_FALSE(model.moveItem(100, 0));
}

TEST_F(QueueModelTest, MoveItemAdjustsCurrentIndex) {
    MediaFileModel file1("/1.mp3");
    MediaFileModel file2("/2.mp3");
    MediaFileModel file3("/3.mp3");
    model.addToEnd(file1);
    model.addToEnd(file2);
    model.addToEnd(file3);
    model.jumpTo(0); // current = 0
    
    // Di chuyển current item từ 0 -> 2
    EXPECT_TRUE(model.moveItem(0, 2));
    EXPECT_EQ(model.getCurrentIndex(), 2);
}

// ===================== Shuffle Mode =====================

TEST_F(QueueModelTest, SetShuffleModeOn) {
    MediaFileModel file1("/1.mp3");
    MediaFileModel file2("/2.mp3");
    model.addToEnd(file1);
    model.addToEnd(file2);
    
    model.setShuffleMode(true);
    EXPECT_TRUE(model.isShuffleEnabled());
    
    // getItemsInPlaybackOrder should return shuffled order
    auto items = model.getItemsInPlaybackOrder();
    EXPECT_EQ(items.size(), 2);
}

TEST_F(QueueModelTest, SetShuffleModeOff) {
    MediaFileModel file1("/1.mp3");
    model.addToEnd(file1);
    model.setShuffleMode(true);
    model.setShuffleMode(false);
    
    EXPECT_FALSE(model.isShuffleEnabled());
}

// ===================== Repeat Mode =====================

TEST_F(QueueModelTest, SetRepeatModeLoopOne) {
    model.setRepeatMode(RepeatMode::LoopOne);
    EXPECT_EQ(model.getRepeatMode(), RepeatMode::LoopOne);
    EXPECT_TRUE(model.isLoopOneEnabled());
    EXPECT_FALSE(model.isLoopAllEnabled());
    EXPECT_TRUE(model.isRepeatEnabled());
}

TEST_F(QueueModelTest, SetRepeatModeLoopAll) {
    model.setRepeatMode(RepeatMode::LoopAll);
    EXPECT_EQ(model.getRepeatMode(), RepeatMode::LoopAll);
    EXPECT_FALSE(model.isLoopOneEnabled());
    EXPECT_TRUE(model.isLoopAllEnabled());
    EXPECT_TRUE(model.isRepeatEnabled());
}

TEST_F(QueueModelTest, SetRepeatModeNone) {
    model.setRepeatMode(RepeatMode::LoopAll);
    model.setRepeatMode(RepeatMode::None);
    EXPECT_EQ(model.getRepeatMode(), RepeatMode::None);
    EXPECT_FALSE(model.isRepeatEnabled());
}

// ===================== LoopAll affect navigation =====================

TEST_F(QueueModelTest, GetNextItemLoopAll) {
    MediaFileModel file1("/1.mp3");
    model.addToEnd(file1);
    model.jumpTo(0);
    model.setRepeatMode(RepeatMode::LoopAll);
    
    // Ở cuối queue với LoopAll, getNextItem wrap về đầu
    auto next = model.getNextItem();
    ASSERT_TRUE(next.has_value());
    EXPECT_EQ(next->getFilePath(), "/1.mp3");
}

TEST_F(QueueModelTest, GetPreviousItemLoopAll) {
    MediaFileModel file1("/1.mp3");
    model.addToEnd(file1);
    model.jumpTo(0);
    model.setRepeatMode(RepeatMode::LoopAll);
    
    // Ở đầu queue với LoopAll, getPreviousItem wrap về cuối
    auto prev = model.getPreviousItem();
    ASSERT_TRUE(prev.has_value());
    EXPECT_EQ(prev->getFilePath(), "/1.mp3");
}

TEST_F(QueueModelTest, HasNextWithLoopAll) {
    MediaFileModel file1("/1.mp3");
    model.addToEnd(file1);
    model.setRepeatMode(RepeatMode::LoopAll);
    
    EXPECT_TRUE(model.hasNext());
}

TEST_F(QueueModelTest, HasPreviousWithLoopAll) {
    MediaFileModel file1("/1.mp3");
    model.addToEnd(file1);
    model.jumpTo(0);
    model.setRepeatMode(RepeatMode::LoopAll);
    
    EXPECT_TRUE(model.hasPrevious());
}

// ===================== moveToNext / moveToPrevious edge cases =====================

TEST_F(QueueModelTest, MoveToNextEmpty) {
    EXPECT_FALSE(model.moveToNext());
}

TEST_F(QueueModelTest, MoveToNextAtEnd) {
    MediaFileModel file1("/1.mp3");
    model.addToEnd(file1);
    model.jumpTo(0);
    
    EXPECT_FALSE(model.moveToNext());
}

TEST_F(QueueModelTest, MoveToNextLoopAll) {
    MediaFileModel file1("/1.mp3");
    MediaFileModel file2("/2.mp3");
    model.addToEnd(file1);
    model.addToEnd(file2);
    model.jumpTo(1);
    model.setRepeatMode(RepeatMode::LoopAll);
    
    EXPECT_TRUE(model.moveToNext());
    EXPECT_EQ(model.getCurrentIndex(), 0);
}

TEST_F(QueueModelTest, MoveToPreviousEmpty) {
    EXPECT_FALSE(model.moveToPrevious());
}

TEST_F(QueueModelTest, MoveToPreviousAtStart) {
    MediaFileModel file1("/1.mp3");
    model.addToEnd(file1);
    model.jumpTo(0);
    
    EXPECT_FALSE(model.moveToPrevious());
}

TEST_F(QueueModelTest, MoveToPreviousLoopAll) {
    MediaFileModel file1("/1.mp3");
    MediaFileModel file2("/2.mp3");
    model.addToEnd(file1);
    model.addToEnd(file2);
    model.jumpTo(0);
    model.setRepeatMode(RepeatMode::LoopAll);
    
    EXPECT_TRUE(model.moveToPrevious());
    EXPECT_EQ(model.getCurrentIndex(), 1);
}

// ===================== jumpTo edge cases =====================

TEST_F(QueueModelTest, JumpToOutOfBounds) {
    MediaFileModel file1("/1.mp3");
    model.addToEnd(file1);
    
    EXPECT_FALSE(model.jumpTo(100));
}

// ===================== removeAt edge cases =====================

TEST_F(QueueModelTest, RemoveAtOutOfBounds) {
    MediaFileModel file1("/1.mp3");
    model.addToEnd(file1);
    
    EXPECT_FALSE(model.removeAt(100));
}

TEST_F(QueueModelTest, RemoveAtAdjustsCurrentIndex) {
    MediaFileModel file1("/1.mp3");
    MediaFileModel file2("/2.mp3");
    MediaFileModel file3("/3.mp3");
    model.addToEnd(file1);
    model.addToEnd(file2);
    model.addToEnd(file3);
    model.jumpTo(2);
    
    // Xóa item cuối khi current đang ở cuối
    model.removeAt(2);
    EXPECT_EQ(model.getCurrentIndex(), 1);
}

// ===================== getItemsInPlaybackOrder =====================

TEST_F(QueueModelTest, GetItemsInPlaybackOrderNoShuffle) {
    MediaFileModel file1("/1.mp3");
    MediaFileModel file2("/2.mp3");
    model.addToEnd(file1);
    model.addToEnd(file2);
    
    auto items = model.getItemsInPlaybackOrder();
    ASSERT_EQ(items.size(), 2);
    EXPECT_EQ(items[0].getFilePath(), "/1.mp3");
    EXPECT_EQ(items[1].getFilePath(), "/2.mp3");
}

TEST_F(QueueModelTest, GetItemsInPlaybackOrderEmpty) {
    auto items = model.getItemsInPlaybackOrder();
    EXPECT_TRUE(items.empty());
}

// ===================== hasNext / hasPrevious empty =====================

TEST_F(QueueModelTest, HasNextEmpty) {
    EXPECT_FALSE(model.hasNext());
}

TEST_F(QueueModelTest, HasPreviousEmpty) {
    EXPECT_FALSE(model.hasPrevious());
}

// ===================== addToEnd with shuffle =====================

TEST_F(QueueModelTest, AddToEndWithShuffleUpdatesOrder) {
    model.setShuffleMode(true);
    MediaFileModel file1("/1.mp3");
    model.addToEnd(file1);
    
    EXPECT_EQ(model.size(), 1);
    auto items = model.getItemsInPlaybackOrder();
    EXPECT_EQ(items.size(), 1);
}

// ===================== removeAt with shuffle =====================

TEST_F(QueueModelTest, RemoveAtWithShuffleUpdatesOrder) {
    MediaFileModel file1("/1.mp3");
    MediaFileModel file2("/2.mp3");
    model.addToEnd(file1);
    model.addToEnd(file2);
    model.setShuffleMode(true);
    
    model.removeAt(0);
    EXPECT_EQ(model.size(), 1);
}

