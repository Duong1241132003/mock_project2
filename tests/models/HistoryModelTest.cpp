#include <gtest/gtest.h>
#include "models/HistoryModel.h"
#include "models/MediaFileModel.h"

#include <thread>
#include <atomic>
#include <vector>

using namespace media_player;

/**
 * @brief Test fixture for HistoryModel tests.
 * 
 * Sets up a fresh HistoryModel instance before each test.
 */
class HistoryModelTest : public testing::Test 
{
protected:
    void SetUp() override 
    {
        // Create HistoryModel without repository for in-memory testing
        historyModel = std::make_shared<models::HistoryModel>(nullptr, 10);
    }
    
    void TearDown() override 
    {
        historyModel.reset();
    }
    
    std::shared_ptr<models::HistoryModel> historyModel;
    
    // Helper to create test media files
    models::MediaFileModel createTestMedia(const std::string& path) 
    {
        return models::MediaFileModel(path);
    }
};

// ============================================================================
// Constructor & Basic State Tests
// ============================================================================

TEST_F(HistoryModelTest, Constructor_InitializesEmpty) 
{
    EXPECT_TRUE(historyModel->isEmpty());
    EXPECT_EQ(historyModel->count(), 0u);
    EXPECT_EQ(historyModel->getMaxEntries(), 10u);
}

TEST_F(HistoryModelTest, Constructor_WithCustomMaxEntries) 
{
    auto customModel = std::make_shared<models::HistoryModel>(nullptr, 50);
    EXPECT_EQ(customModel->getMaxEntries(), 50u);
}

// ============================================================================
// AddEntry Tests
// ============================================================================

TEST_F(HistoryModelTest, AddEntry_AddsToFront) 
{
    auto media1 = createTestMedia("/tmp/song1.mp3");
    auto media2 = createTestMedia("/tmp/song2.mp3");
    
    historyModel->addEntry(media1);
    historyModel->addEntry(media2);
    
    EXPECT_EQ(historyModel->count(), 2u);
    
    auto lastPlayed = historyModel->getLastPlayed();
    ASSERT_TRUE(lastPlayed.has_value());
    EXPECT_EQ(lastPlayed->media.getFilePath(), "/tmp/song2.mp3");
}

TEST_F(HistoryModelTest, AddEntry_MaxEntriesLimit) 
{
    // Add 15 entries to a model with max 10
    for (int i = 0; i < 15; ++i) 
    {
        auto media = createTestMedia("/tmp/song" + std::to_string(i) + ".mp3");
        historyModel->addEntry(media);
    }
    
    // Should only keep last 10
    EXPECT_EQ(historyModel->count(), 10u);
    
    // Most recent should be song14
    auto lastPlayed = historyModel->getLastPlayed();
    ASSERT_TRUE(lastPlayed.has_value());
    EXPECT_EQ(lastPlayed->media.getFilePath(), "/tmp/song14.mp3");
}

TEST_F(HistoryModelTest, AddEntry_AllowsDuplicates) 
{
    auto media = createTestMedia("/tmp/song.mp3");
    
    historyModel->addEntry(media);
    historyModel->addEntry(media);
    
    // Should have 2 entries (duplicates allowed)
    EXPECT_EQ(historyModel->count(), 2u);
}

// ============================================================================
// RemoveEntry Tests
// ============================================================================

TEST_F(HistoryModelTest, RemoveMostRecentEntry_RemovesCorrectly) 
{
    auto media1 = createTestMedia("/tmp/song1.mp3");
    auto media2 = createTestMedia("/tmp/song2.mp3");
    
    historyModel->addEntry(media1);
    historyModel->addEntry(media2);
    historyModel->addEntry(media1); // Add media1 again (most recent)
    
    EXPECT_EQ(historyModel->count(), 3u);
    
    bool removed = historyModel->removeMostRecentEntry("/tmp/song1.mp3");
    EXPECT_TRUE(removed);
    EXPECT_EQ(historyModel->count(), 2u);
    
    // After removal, song2 should be most recent
    auto lastPlayed = historyModel->getLastPlayed();
    ASSERT_TRUE(lastPlayed.has_value());
    EXPECT_EQ(lastPlayed->media.getFilePath(), "/tmp/song2.mp3");
}

TEST_F(HistoryModelTest, RemoveMostRecentEntry_NotFound_ReturnsFalse) 
{
    auto media = createTestMedia("/tmp/song.mp3");
    historyModel->addEntry(media);
    
    bool removed = historyModel->removeMostRecentEntry("/tmp/nonexistent.mp3");
    EXPECT_FALSE(removed);
    EXPECT_EQ(historyModel->count(), 1u);
}

TEST_F(HistoryModelTest, RemoveAllEntries_RemovesAllMatches) 
{
    auto media1 = createTestMedia("/tmp/song1.mp3");
    auto media2 = createTestMedia("/tmp/song2.mp3");
    
    historyModel->addEntry(media1);
    historyModel->addEntry(media2);
    historyModel->addEntry(media1);
    historyModel->addEntry(media2);
    historyModel->addEntry(media1);
    
    size_t removedCount = historyModel->removeAllEntries("/tmp/song1.mp3");
    EXPECT_EQ(removedCount, 3u);
    EXPECT_EQ(historyModel->count(), 2u);
}

TEST_F(HistoryModelTest, RemoveAllEntries_NotFound_ReturnsZero) 
{
    auto media = createTestMedia("/tmp/song.mp3");
    historyModel->addEntry(media);
    
    size_t removedCount = historyModel->removeAllEntries("/tmp/nonexistent.mp3");
    EXPECT_EQ(removedCount, 0u);
    EXPECT_EQ(historyModel->count(), 1u);
}

// ============================================================================
// Clear Tests
// ============================================================================

TEST_F(HistoryModelTest, Clear_RemovesAllEntries) 
{
    for (int i = 0; i < 5; ++i) 
    {
        historyModel->addEntry(createTestMedia("/tmp/song" + std::to_string(i) + ".mp3"));
    }
    
    EXPECT_EQ(historyModel->count(), 5u);
    
    historyModel->clear();
    
    EXPECT_TRUE(historyModel->isEmpty());
    EXPECT_EQ(historyModel->count(), 0u);
}

// ============================================================================
// Query Operations Tests
// ============================================================================

TEST_F(HistoryModelTest, GetRecentHistory_ReturnsCorrectCount) 
{
    for (int i = 0; i < 5; ++i) 
    {
        historyModel->addEntry(createTestMedia("/tmp/song" + std::to_string(i) + ".mp3"));
    }
    
    auto recent = historyModel->getRecentHistory(3);
    EXPECT_EQ(recent.size(), 3u);
    
    // Most recent first
    EXPECT_EQ(recent[0].media.getFilePath(), "/tmp/song4.mp3");
    EXPECT_EQ(recent[1].media.getFilePath(), "/tmp/song3.mp3");
    EXPECT_EQ(recent[2].media.getFilePath(), "/tmp/song2.mp3");
}

TEST_F(HistoryModelTest, GetRecentHistory_LessThanRequested) 
{
    historyModel->addEntry(createTestMedia("/tmp/song.mp3"));
    
    auto recent = historyModel->getRecentHistory(10);
    EXPECT_EQ(recent.size(), 1u);
}

TEST_F(HistoryModelTest, GetAllHistory_ReturnsAll) 
{
    for (int i = 0; i < 5; ++i) 
    {
        historyModel->addEntry(createTestMedia("/tmp/song" + std::to_string(i) + ".mp3"));
    }
    
    auto all = historyModel->getAllHistory();
    EXPECT_EQ(all.size(), 5u);
}

TEST_F(HistoryModelTest, GetEntryAt_ValidIndex_ReturnsEntry) 
{
    historyModel->addEntry(createTestMedia("/tmp/song1.mp3"));
    historyModel->addEntry(createTestMedia("/tmp/song2.mp3"));
    
    auto entry = historyModel->getEntryAt(1);
    ASSERT_TRUE(entry.has_value());
    EXPECT_EQ(entry->media.getFilePath(), "/tmp/song1.mp3");
}

TEST_F(HistoryModelTest, GetEntryAt_InvalidIndex_ReturnsNullopt) 
{
    historyModel->addEntry(createTestMedia("/tmp/song.mp3"));
    
    auto entry = historyModel->getEntryAt(5);
    EXPECT_FALSE(entry.has_value());
}

TEST_F(HistoryModelTest, GetLastPlayed_Empty_ReturnsNullopt) 
{
    auto lastPlayed = historyModel->getLastPlayed();
    EXPECT_FALSE(lastPlayed.has_value());
}

TEST_F(HistoryModelTest, GetPreviousPlayed_ReturnsSecondMostRecent) 
{
    historyModel->addEntry(createTestMedia("/tmp/song1.mp3"));
    historyModel->addEntry(createTestMedia("/tmp/song2.mp3"));
    
    auto previous = historyModel->getPreviousPlayed();
    ASSERT_TRUE(previous.has_value());
    EXPECT_EQ(previous->media.getFilePath(), "/tmp/song1.mp3");
}

TEST_F(HistoryModelTest, GetPreviousPlayed_OnlyOneEntry_ReturnsNullopt) 
{
    historyModel->addEntry(createTestMedia("/tmp/song.mp3"));
    
    auto previous = historyModel->getPreviousPlayed();
    EXPECT_FALSE(previous.has_value());
}

TEST_F(HistoryModelTest, GetPreviousPlayed_Empty_ReturnsNullopt) 
{
    auto previous = historyModel->getPreviousPlayed();
    EXPECT_FALSE(previous.has_value());
}

// ============================================================================
// WasRecentlyPlayed Tests
// ============================================================================

TEST_F(HistoryModelTest, WasRecentlyPlayed_JustPlayed_ReturnsTrue) 
{
    historyModel->addEntry(createTestMedia("/tmp/song.mp3"));
    
    EXPECT_TRUE(historyModel->wasRecentlyPlayed("/tmp/song.mp3", 30));
}

TEST_F(HistoryModelTest, WasRecentlyPlayed_NotInHistory_ReturnsFalse) 
{
    EXPECT_FALSE(historyModel->wasRecentlyPlayed("/tmp/nonexistent.mp3", 30));
}

// ============================================================================
// Thread Safety Tests
// ============================================================================

TEST_F(HistoryModelTest, ThreadSafety_ConcurrentAdditions) 
{
    std::atomic<int> addedCount{0};
    std::vector<std::thread> threads;
    
    // Launch multiple threads adding entries
    for (int t = 0; t < 4; ++t) 
    {
        threads.emplace_back([this, t, &addedCount]() 
        {
            for (int i = 0; i < 25; ++i) 
            {
                auto media = createTestMedia("/tmp/thread" + std::to_string(t) + "_song" + std::to_string(i) + ".mp3");
                historyModel->addEntry(media);
                addedCount++;
            }
        });
    }
    
    // Wait for all threads
    for (auto& thread : threads) 
    {
        thread.join();
    }
    
    // All adds should have completed without crashes
    // Count should be maxEntries since we added more than 10
    EXPECT_EQ(historyModel->count(), 10u);
}

TEST_F(HistoryModelTest, ThreadSafety_ConcurrentReadsAndWrites) 
{
    // Pre-populate
    for (int i = 0; i < 5; ++i) 
    {
        historyModel->addEntry(createTestMedia("/tmp/song" + std::to_string(i) + ".mp3"));
    }
    
    std::atomic<bool> done{false};
    std::thread writer([this, &done]() 
    {
        while (!done) 
        {
            historyModel->addEntry(createTestMedia("/tmp/new_song.mp3"));
            historyModel->removeMostRecentEntry("/tmp/new_song.mp3");
        }
    });
    
    std::thread reader([this, &done]() 
    {
        while (!done) 
        {
            auto all = historyModel->getAllHistory();
            auto count = historyModel->count();
            auto last = historyModel->getLastPlayed();
            (void)all; (void)count; (void)last;
        }
    });
    
    // Run for a short time
    std::this_thread::sleep_for(std::chrono::milliseconds(100));
    done = true;
    
    writer.join();
    reader.join();
    
    // No crashes means success
    EXPECT_TRUE(true);
}

// ============================================================================
// State Query Tests
// ============================================================================

TEST_F(HistoryModelTest, IsEmpty_NewModel_ReturnsTrue) 
{
    EXPECT_TRUE(historyModel->isEmpty());
}

TEST_F(HistoryModelTest, IsEmpty_AfterAdd_ReturnsFalse) 
{
    historyModel->addEntry(createTestMedia("/tmp/song.mp3"));
    EXPECT_FALSE(historyModel->isEmpty());
}

TEST_F(HistoryModelTest, IsEmpty_AfterClear_ReturnsTrue) 
{
    historyModel->addEntry(createTestMedia("/tmp/song.mp3"));
    historyModel->clear();
    EXPECT_TRUE(historyModel->isEmpty());
}

// ============================================================================
// Persistence Tests (without repository)
// ============================================================================

TEST_F(HistoryModelTest, LoadFromRepository_NoRepository_ReturnsFalse) 
{
    EXPECT_FALSE(historyModel->loadFromRepository());
}

TEST_F(HistoryModelTest, SaveToRepository_NoRepository_ReturnsFalse) 
{
    EXPECT_FALSE(historyModel->saveToRepository());
}
