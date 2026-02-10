/**
 * @file HistoryRepositoryTest.cpp
 * @brief Unit tests cho HistoryRepository — quản lý lịch sử phát nhạc
 *
 * Bao gồm: addEntry, remove, getRecentHistory, getAllHistory, setHistory,
 * clear, count, wasRecentlyPlayed, getLastPlayed, getPreviousPlayed,
 * getPlayedBefore, serialize/deserialize round-trip, edge cases.
 */

#include <gtest/gtest.h>
#include "repositories/HistoryRepository.h"
#include "models/MediaFileModel.h"

#include <filesystem>
#include <fstream>
#include <thread>
#include <chrono>

namespace fs = std::filesystem;
using namespace media_player::repositories;
using namespace media_player::models;

// ============================================================================
// Test Fixture
// ============================================================================

class HistoryRepositoryTest : public ::testing::Test
{
protected:
    void SetUp() override
    {
        // Tạo thư mục tạm cho test
        m_testDir = fs::temp_directory_path() / "MediaPlayerTest_HistoryRepo";
        if (fs::exists(m_testDir))
        {
            fs::remove_all(m_testDir);
        }
        fs::create_directories(m_testDir);

        // Tạo dummy media files để MediaFileModel.isValid() trả về true
        createDummyFile(m_testDir / "song1.mp3");
        createDummyFile(m_testDir / "song2.mp3");
        createDummyFile(m_testDir / "song3.mp3");
        createDummyFile(m_testDir / "song4.mp3");
        createDummyFile(m_testDir / "song5.mp3");

        m_storagePath = (m_testDir / "history_storage").string();
    }

    void TearDown() override
    {
        if (fs::exists(m_testDir))
        {
            fs::remove_all(m_testDir);
        }
    }

    /// Tạo file giả
    void createDummyFile(const fs::path& path)
    {
        std::ofstream ofs(path);
        ofs << "dummy";
        ofs.close();
    }

    /// Tạo MediaFileModel từ dummy file
    MediaFileModel makeMedia(int index)
    {
        return MediaFileModel((m_testDir / ("song" + std::to_string(index) + ".mp3")).string());
    }

    fs::path m_testDir;
    std::string m_storagePath;
};

// ============================================================================
// Constructor & Basic State
// ============================================================================

TEST_F(HistoryRepositoryTest, ConstructorCreatesStorageDirectory)
{
    HistoryRepository repo(m_storagePath);
    EXPECT_TRUE(fs::exists(m_storagePath));
}

TEST_F(HistoryRepositoryTest, InitialStateEmpty)
{
    HistoryRepository repo(m_storagePath);
    EXPECT_EQ(repo.count(), 0);
    EXPECT_TRUE(repo.getAllHistory().empty());
}

// ============================================================================
// addEntry
// ============================================================================

TEST_F(HistoryRepositoryTest, AddEntryIncreasesCount)
{
    HistoryRepository repo(m_storagePath);
    repo.addEntry(makeMedia(1));

    EXPECT_EQ(repo.count(), 1);
}

TEST_F(HistoryRepositoryTest, AddEntryMultiple)
{
    HistoryRepository repo(m_storagePath);
    repo.addEntry(makeMedia(1));
    repo.addEntry(makeMedia(2));
    repo.addEntry(makeMedia(3));

    EXPECT_EQ(repo.count(), 3);
}

TEST_F(HistoryRepositoryTest, AddEntryPushesToFront)
{
    // Entry mới nhất nằm ở đầu
    HistoryRepository repo(m_storagePath);
    repo.addEntry(makeMedia(1));
    repo.addEntry(makeMedia(2));

    auto history = repo.getAllHistory();
    ASSERT_EQ(history.size(), 2);
    // song2 được thêm sau -> nằm đầu
    EXPECT_TRUE(history[0].media.getFilePath().find("song2") != std::string::npos);
    EXPECT_TRUE(history[1].media.getFilePath().find("song1") != std::string::npos);
}

TEST_F(HistoryRepositoryTest, AddEntryRespectsMaxEntries)
{
    // maxEntries = 3
    HistoryRepository repo(m_storagePath, 3);
    repo.addEntry(makeMedia(1));
    repo.addEntry(makeMedia(2));
    repo.addEntry(makeMedia(3));
    repo.addEntry(makeMedia(4));

    EXPECT_EQ(repo.count(), 3);
    // song1 (cũ nhất) bị loại
    auto history = repo.getAllHistory();
    for (const auto& entry : history)
    {
        EXPECT_TRUE(entry.media.getFilePath().find("song1") == std::string::npos);
    }
}

// ============================================================================
// removeMostRecentEntryByFilePath
// ============================================================================

TEST_F(HistoryRepositoryTest, RemoveMostRecentEntryByFilePath)
{
    HistoryRepository repo(m_storagePath);
    auto media1 = makeMedia(1);
    repo.addEntry(media1);
    repo.addEntry(makeMedia(2));
    repo.addEntry(media1);  // Thêm lại song1

    // Chỉ xóa entry gần nhất (đầu tiên tìm thấy)
    repo.removeMostRecentEntryByFilePath(media1.getFilePath());

    // Vẫn còn 1 entry của song1 và 1 entry của song2
    EXPECT_EQ(repo.count(), 2);

    auto history = repo.getAllHistory();
    int song1Count = 0;
    for (const auto& entry : history)
    {
        if (entry.media.getFilePath() == media1.getFilePath())
        {
            song1Count++;
        }
    }
    EXPECT_EQ(song1Count, 1);
}

TEST_F(HistoryRepositoryTest, RemoveMostRecentEntryByFilePathNotFound)
{
    HistoryRepository repo(m_storagePath);
    repo.addEntry(makeMedia(1));

    // Xóa path không tồn tại — không crash
    repo.removeMostRecentEntryByFilePath("/nonexistent.mp3");
    EXPECT_EQ(repo.count(), 1);
}

// ============================================================================
// removeAllEntriesByFilePath
// ============================================================================

TEST_F(HistoryRepositoryTest, RemoveAllEntriesByFilePath)
{
    HistoryRepository repo(m_storagePath);
    auto media1 = makeMedia(1);
    repo.addEntry(media1);
    repo.addEntry(makeMedia(2));
    repo.addEntry(media1);

    repo.removeAllEntriesByFilePath(media1.getFilePath());

    EXPECT_EQ(repo.count(), 1);

    auto history = repo.getAllHistory();
    EXPECT_TRUE(history[0].media.getFilePath().find("song2") != std::string::npos);
}

TEST_F(HistoryRepositoryTest, RemoveAllEntriesByFilePathNotFound)
{
    HistoryRepository repo(m_storagePath);
    repo.addEntry(makeMedia(1));

    repo.removeAllEntriesByFilePath("/nonexistent.mp3");
    EXPECT_EQ(repo.count(), 1);
}

// ============================================================================
// getRecentHistory
// ============================================================================

TEST_F(HistoryRepositoryTest, GetRecentHistoryLessThanCount)
{
    HistoryRepository repo(m_storagePath);
    repo.addEntry(makeMedia(1));

    auto recent = repo.getRecentHistory(5);
    EXPECT_EQ(recent.size(), 1);
}

TEST_F(HistoryRepositoryTest, GetRecentHistoryExactCount)
{
    HistoryRepository repo(m_storagePath);
    repo.addEntry(makeMedia(1));
    repo.addEntry(makeMedia(2));
    repo.addEntry(makeMedia(3));

    auto recent = repo.getRecentHistory(3);
    EXPECT_EQ(recent.size(), 3);
}

TEST_F(HistoryRepositoryTest, GetRecentHistoryTruncated)
{
    HistoryRepository repo(m_storagePath);
    repo.addEntry(makeMedia(1));
    repo.addEntry(makeMedia(2));
    repo.addEntry(makeMedia(3));

    auto recent = repo.getRecentHistory(2);
    EXPECT_EQ(recent.size(), 2);
    // Hai entry gần nhất
    EXPECT_TRUE(recent[0].media.getFilePath().find("song3") != std::string::npos);
    EXPECT_TRUE(recent[1].media.getFilePath().find("song2") != std::string::npos);
}

TEST_F(HistoryRepositoryTest, GetRecentHistoryEmpty)
{
    HistoryRepository repo(m_storagePath);
    auto recent = repo.getRecentHistory(5);
    EXPECT_TRUE(recent.empty());
}

// ============================================================================
// setHistory
// ============================================================================

TEST_F(HistoryRepositoryTest, SetHistoryReplacesAll)
{
    HistoryRepository repo(m_storagePath);
    repo.addEntry(makeMedia(1));

    std::vector<PlaybackHistoryEntry> newHistory;
    newHistory.emplace_back(makeMedia(2));
    newHistory.emplace_back(makeMedia(3));

    repo.setHistory(newHistory);

    EXPECT_EQ(repo.count(), 2);
    auto all = repo.getAllHistory();
    EXPECT_TRUE(all[0].media.getFilePath().find("song2") != std::string::npos);
}

// ============================================================================
// clear & count
// ============================================================================

TEST_F(HistoryRepositoryTest, ClearRemovesAll)
{
    HistoryRepository repo(m_storagePath);
    repo.addEntry(makeMedia(1));
    repo.addEntry(makeMedia(2));

    repo.clear();

    EXPECT_EQ(repo.count(), 0);
    EXPECT_TRUE(repo.getAllHistory().empty());
}

// ============================================================================
// wasRecentlyPlayed
// ============================================================================

TEST_F(HistoryRepositoryTest, WasRecentlyPlayedTrue)
{
    HistoryRepository repo(m_storagePath);
    auto media1 = makeMedia(1);
    repo.addEntry(media1);

    // Vừa mới thêm → chắc chắn recently played
    EXPECT_TRUE(repo.wasRecentlyPlayed(media1.getFilePath(), 30));
}

TEST_F(HistoryRepositoryTest, WasRecentlyPlayedFalseNotInHistory)
{
    HistoryRepository repo(m_storagePath);
    repo.addEntry(makeMedia(1));

    EXPECT_FALSE(repo.wasRecentlyPlayed("/nonexistent.mp3", 30));
}

TEST_F(HistoryRepositoryTest, WasRecentlyPlayedFalseEmpty)
{
    HistoryRepository repo(m_storagePath);
    EXPECT_FALSE(repo.wasRecentlyPlayed("/any.mp3", 30));
}

// ============================================================================
// getLastPlayed
// ============================================================================

TEST_F(HistoryRepositoryTest, GetLastPlayedReturnsNewest)
{
    HistoryRepository repo(m_storagePath);
    repo.addEntry(makeMedia(1));
    repo.addEntry(makeMedia(2));

    auto last = repo.getLastPlayed();
    ASSERT_TRUE(last.has_value());
    EXPECT_TRUE(last->media.getFilePath().find("song2") != std::string::npos);
}

TEST_F(HistoryRepositoryTest, GetLastPlayedEmpty)
{
    HistoryRepository repo(m_storagePath);
    EXPECT_FALSE(repo.getLastPlayed().has_value());
}

// ============================================================================
// getPreviousPlayed
// ============================================================================

TEST_F(HistoryRepositoryTest, GetPreviousPlayedReturnsSecond)
{
    HistoryRepository repo(m_storagePath);
    repo.addEntry(makeMedia(1));
    repo.addEntry(makeMedia(2));

    auto prev = repo.getPreviousPlayed();
    ASSERT_TRUE(prev.has_value());
    EXPECT_TRUE(prev->media.getFilePath().find("song1") != std::string::npos);
}

TEST_F(HistoryRepositoryTest, GetPreviousPlayedOnlyOneEntry)
{
    HistoryRepository repo(m_storagePath);
    repo.addEntry(makeMedia(1));

    EXPECT_FALSE(repo.getPreviousPlayed().has_value());
}

TEST_F(HistoryRepositoryTest, GetPreviousPlayedEmpty)
{
    HistoryRepository repo(m_storagePath);
    EXPECT_FALSE(repo.getPreviousPlayed().has_value());
}

// ============================================================================
// getPlayedBefore
// ============================================================================

TEST_F(HistoryRepositoryTest, GetPlayedBeforeFound)
{
    HistoryRepository repo(m_storagePath);
    auto media1 = makeMedia(1);
    auto media2 = makeMedia(2);
    auto media3 = makeMedia(3);
    repo.addEntry(media1);  // index 2
    repo.addEntry(media2);  // index 1
    repo.addEntry(media3);  // index 0

    // Tìm entry trước media3 → media2
    auto before = repo.getPlayedBefore(media3.getFilePath());
    ASSERT_TRUE(before.has_value());
    EXPECT_TRUE(before->media.getFilePath().find("song2") != std::string::npos);
}

TEST_F(HistoryRepositoryTest, GetPlayedBeforeNotFoundLastEntry)
{
    // Entry cuối cùng không có entry trước nó
    HistoryRepository repo(m_storagePath);
    auto media1 = makeMedia(1);
    repo.addEntry(media1);

    auto before = repo.getPlayedBefore(media1.getFilePath());
    EXPECT_FALSE(before.has_value());
}

TEST_F(HistoryRepositoryTest, GetPlayedBeforeNotFoundNotInHistory)
{
    HistoryRepository repo(m_storagePath);
    repo.addEntry(makeMedia(1));

    auto before = repo.getPlayedBefore("/nonexistent.mp3");
    EXPECT_FALSE(before.has_value());
}

// ============================================================================
// Serialize / Deserialize Round-Trip
// ============================================================================

TEST_F(HistoryRepositoryTest, SaveAndLoadRoundTrip)
{
    {
        // Tạo repo, thêm dữ liệu, destructor sẽ saveToDisk
        HistoryRepository repo(m_storagePath);
        auto media1 = makeMedia(1);
        media1.setTitle("Test Title");
        media1.setArtist("Test Artist");
        repo.addEntry(media1);
        repo.addEntry(makeMedia(2));
    }

    // Tạo repo mới → constructor loadFromDisk
    HistoryRepository repo2(m_storagePath);
    EXPECT_EQ(repo2.count(), 2);

    auto history = repo2.getAllHistory();
    ASSERT_EQ(history.size(), 2);
}

TEST_F(HistoryRepositoryTest, LoadFromEmptyStorage)
{
    // Không có file nào → load không lỗi, count = 0
    HistoryRepository repo(m_storagePath);
    EXPECT_EQ(repo.count(), 0);
}

TEST_F(HistoryRepositoryTest, SaveToDiskExplicit)
{
    HistoryRepository repo(m_storagePath);
    repo.addEntry(makeMedia(1));

    EXPECT_TRUE(repo.saveToDisk());
    EXPECT_TRUE(fs::exists(m_storagePath + "/history.dat"));
}

// ============================================================================
// getAllHistory
// ============================================================================

TEST_F(HistoryRepositoryTest, GetAllHistoryOrder)
{
    HistoryRepository repo(m_storagePath);
    repo.addEntry(makeMedia(1));
    repo.addEntry(makeMedia(2));
    repo.addEntry(makeMedia(3));

    auto all = repo.getAllHistory();
    ASSERT_EQ(all.size(), 3);
    // Thứ tự: mới nhất trước
    EXPECT_TRUE(all[0].media.getFilePath().find("song3") != std::string::npos);
    EXPECT_TRUE(all[1].media.getFilePath().find("song2") != std::string::npos);
    EXPECT_TRUE(all[2].media.getFilePath().find("song1") != std::string::npos);
}

// ============================================================================
// Legacy Deserialization (pipe-separated format)
// ============================================================================

TEST_F(HistoryRepositoryTest, LoadLegacyPipeFormat)
{
    // Tạo file history với format legacy: path|timestamp
    fs::create_directories(m_storagePath);
    std::string historyFile = m_storagePath + "/history.dat";

    auto media1 = makeMedia(1);
    auto now = std::chrono::system_clock::now();
    auto timestamp = std::chrono::system_clock::to_time_t(now);

    {
        std::ofstream file(historyFile);
        file << "HISTORY_VERSION:1.0\n";
        file << "COUNT:1\n";
        file << "ENTRIES:\n";
        file << media1.getFilePath() << "|" << timestamp << "\n";
        file.close();
    }

    // Tạo repo mới — constructor load legacy format
    HistoryRepository repo(m_storagePath);
    EXPECT_EQ(repo.count(), 1);
}

// ============================================================================
// loadFromDisk explicit
// ============================================================================

TEST_F(HistoryRepositoryTest, LoadFromDiskExplicit)
{
    HistoryRepository repo(m_storagePath);
    repo.addEntry(makeMedia(1));
    repo.saveToDisk();

    EXPECT_TRUE(repo.loadFromDisk());
}

// ============================================================================
// Metadata preservation in serialization
// ============================================================================

TEST_F(HistoryRepositoryTest, SerializationPreservesMetadata)
{
    auto media = makeMedia(1);
    media.setTitle("My Song Title");
    media.setArtist("My Artist");

    {
        HistoryRepository repo(m_storagePath);
        repo.addEntry(media);
    }

    HistoryRepository repo2(m_storagePath);
    EXPECT_EQ(repo2.count(), 1);

    auto history = repo2.getAllHistory();
    ASSERT_EQ(history.size(), 1);
    EXPECT_EQ(history[0].media.getTitle(), "My Song Title");
    EXPECT_EQ(history[0].media.getArtist(), "My Artist");
}
