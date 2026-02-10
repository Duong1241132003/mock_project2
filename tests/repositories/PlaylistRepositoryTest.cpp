/**
 * @file PlaylistRepositoryTest.cpp
 * @brief Unit tests cho PlaylistRepository — quản lý playlist trên đĩa
 *
 * Bao gồm: save, findById, findAll, update, remove, exists,
 * saveAll, clear, count, findByName, searchByName,
 * serialize/deserialize round-trip, edge cases.
 */

#include <gtest/gtest.h>
#include "repositories/PlaylistRepository.h"
#include "models/PlaylistModel.h"
#include "models/MediaFileModel.h"

#include <filesystem>
#include <fstream>

namespace fs = std::filesystem;
using namespace media_player::repositories;
using namespace media_player::models;

// ============================================================================
// Test Fixture
// ============================================================================

class PlaylistRepositoryTest : public ::testing::Test
{
protected:
    void SetUp() override
    {
        // Tạo thư mục tạm cho test
        m_testDir = fs::temp_directory_path() / "MediaPlayerTest_PlaylistRepo";
        if (fs::exists(m_testDir))
        {
            fs::remove_all(m_testDir);
        }
        fs::create_directories(m_testDir);

        // Tạo dummy media files cho playlist items
        createDummyFile(m_testDir / "song1.mp3");
        createDummyFile(m_testDir / "song2.mp3");
        createDummyFile(m_testDir / "song3.mp3");

        m_storagePath = (m_testDir / "playlist_storage").string();
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
        ofs << "dummy content";
        ofs.close();
    }

    /// Tạo MediaFileModel
    MediaFileModel makeMedia(const std::string& name)
    {
        return MediaFileModel((m_testDir / name).string());
    }

    /// Tạo PlaylistModel với ID và items
    PlaylistModel makePlaylist(const std::string& name, const std::string& id = "")
    {
        PlaylistModel playlist(name);
        if (!id.empty())
        {
            playlist.setId(id);
        }
        return playlist;
    }

    fs::path m_testDir;
    std::string m_storagePath;
};

// ============================================================================
// Constructor & Initial State
// ============================================================================

TEST_F(PlaylistRepositoryTest, ConstructorCreatesStorageDirectory)
{
    PlaylistRepository repo(m_storagePath);
    EXPECT_TRUE(fs::exists(m_storagePath));
}

TEST_F(PlaylistRepositoryTest, InitialStateEmpty)
{
    PlaylistRepository repo(m_storagePath);
    EXPECT_EQ(repo.count(), 0);
    EXPECT_TRUE(repo.findAll().empty());
}

// ============================================================================
// save
// ============================================================================

TEST_F(PlaylistRepositoryTest, SaveValid)
{
    PlaylistRepository repo(m_storagePath);
    auto playlist = makePlaylist("My Playlist", "pl_1");

    EXPECT_TRUE(repo.save(playlist));
    EXPECT_EQ(repo.count(), 1);
}

TEST_F(PlaylistRepositoryTest, SaveWithEmptyIdFails)
{
    PlaylistRepository repo(m_storagePath);
    PlaylistModel playlist("Test");
    playlist.setId("");  // Đảm bảo ID rỗng

    EXPECT_FALSE(repo.save(playlist));
}

TEST_F(PlaylistRepositoryTest, SaveMultiple)
{
    PlaylistRepository repo(m_storagePath);
    repo.save(makePlaylist("Playlist A", "pl_a"));
    repo.save(makePlaylist("Playlist B", "pl_b"));
    repo.save(makePlaylist("Playlist C", "pl_c"));

    EXPECT_EQ(repo.count(), 3);
}

TEST_F(PlaylistRepositoryTest, SaveWithItems)
{
    PlaylistRepository repo(m_storagePath);
    auto playlist = makePlaylist("With Songs", "pl_songs");
    auto media1 = makeMedia("song1.mp3");
    media1.setTitle("Song One");
    media1.setArtist("Artist One");
    playlist.addItem(media1);
    playlist.addItem(makeMedia("song2.mp3"));

    EXPECT_TRUE(repo.save(playlist));
    EXPECT_EQ(repo.count(), 1);
}

// ============================================================================
// findById
// ============================================================================

TEST_F(PlaylistRepositoryTest, FindByIdFound)
{
    PlaylistRepository repo(m_storagePath);
    auto playlist = makePlaylist("Test PL", "pl_find");
    repo.save(playlist);

    auto found = repo.findById("pl_find");
    ASSERT_TRUE(found.has_value());
    EXPECT_EQ(found->getName(), "Test PL");
}

TEST_F(PlaylistRepositoryTest, FindByIdNotFound)
{
    PlaylistRepository repo(m_storagePath);
    auto found = repo.findById("nonexistent");
    EXPECT_FALSE(found.has_value());
}

// ============================================================================
// findAll
// ============================================================================

TEST_F(PlaylistRepositoryTest, FindAllReturnsAll)
{
    PlaylistRepository repo(m_storagePath);
    repo.save(makePlaylist("PL1", "pl_1"));
    repo.save(makePlaylist("PL2", "pl_2"));

    auto all = repo.findAll();
    EXPECT_EQ(all.size(), 2);
}

TEST_F(PlaylistRepositoryTest, FindAllWhenEmpty)
{
    PlaylistRepository repo(m_storagePath);
    auto all = repo.findAll();
    EXPECT_TRUE(all.empty());
}

// ============================================================================
// update
// ============================================================================

TEST_F(PlaylistRepositoryTest, UpdateExisting)
{
    PlaylistRepository repo(m_storagePath);
    auto playlist = makePlaylist("Original", "pl_upd");
    repo.save(playlist);

    // Update name
    playlist.setName("Updated");
    EXPECT_TRUE(repo.update(playlist));

    auto found = repo.findById("pl_upd");
    ASSERT_TRUE(found.has_value());
    EXPECT_EQ(found->getName(), "Updated");
}

TEST_F(PlaylistRepositoryTest, UpdateNonExisting)
{
    PlaylistRepository repo(m_storagePath);
    auto playlist = makePlaylist("Ghost", "pl_ghost");

    EXPECT_FALSE(repo.update(playlist));
}

// ============================================================================
// remove
// ============================================================================

TEST_F(PlaylistRepositoryTest, RemoveExisting)
{
    PlaylistRepository repo(m_storagePath);
    repo.save(makePlaylist("ToDelete", "pl_del"));

    EXPECT_TRUE(repo.remove("pl_del"));
    EXPECT_EQ(repo.count(), 0);
    EXPECT_FALSE(repo.findById("pl_del").has_value());
}

TEST_F(PlaylistRepositoryTest, RemoveNonExisting)
{
    PlaylistRepository repo(m_storagePath);
    EXPECT_FALSE(repo.remove("nonexistent"));
}

TEST_F(PlaylistRepositoryTest, RemoveDoesNotAffectOthers)
{
    PlaylistRepository repo(m_storagePath);
    repo.save(makePlaylist("Keep", "pl_keep"));
    repo.save(makePlaylist("Delete", "pl_del"));

    repo.remove("pl_del");

    EXPECT_EQ(repo.count(), 1);
    EXPECT_TRUE(repo.findById("pl_keep").has_value());
}

// ============================================================================
// exists
// ============================================================================

TEST_F(PlaylistRepositoryTest, ExistsTrue)
{
    PlaylistRepository repo(m_storagePath);
    repo.save(makePlaylist("Exists", "pl_exists"));

    EXPECT_TRUE(repo.exists("pl_exists"));
}

TEST_F(PlaylistRepositoryTest, ExistsFalse)
{
    PlaylistRepository repo(m_storagePath);
    EXPECT_FALSE(repo.exists("pl_nope"));
}

// ============================================================================
// saveAll
// ============================================================================

TEST_F(PlaylistRepositoryTest, SaveAllMultiple)
{
    PlaylistRepository repo(m_storagePath);

    std::vector<PlaylistModel> playlists;
    playlists.push_back(makePlaylist("PL A", "pl_a"));
    playlists.push_back(makePlaylist("PL B", "pl_b"));

    EXPECT_TRUE(repo.saveAll(playlists));
    EXPECT_EQ(repo.count(), 2);
}

TEST_F(PlaylistRepositoryTest, SaveAllEmpty)
{
    PlaylistRepository repo(m_storagePath);

    std::vector<PlaylistModel> emptyList;
    EXPECT_TRUE(repo.saveAll(emptyList));
    EXPECT_EQ(repo.count(), 0);
}

// ============================================================================
// clear
// ============================================================================

TEST_F(PlaylistRepositoryTest, ClearRemovesAll)
{
    PlaylistRepository repo(m_storagePath);
    repo.save(makePlaylist("PL1", "pl_1"));
    repo.save(makePlaylist("PL2", "pl_2"));

    repo.clear();

    EXPECT_EQ(repo.count(), 0);
    EXPECT_TRUE(repo.findAll().empty());
}

// ============================================================================
// findByName
// ============================================================================

TEST_F(PlaylistRepositoryTest, FindByNameFound)
{
    PlaylistRepository repo(m_storagePath);
    repo.save(makePlaylist("Rock Hits", "pl_rock"));
    repo.save(makePlaylist("Jazz Vibes", "pl_jazz"));

    auto found = repo.findByName("Jazz Vibes");
    ASSERT_TRUE(found.has_value());
    EXPECT_EQ(found->getId(), "pl_jazz");
}

TEST_F(PlaylistRepositoryTest, FindByNameNotFound)
{
    PlaylistRepository repo(m_storagePath);
    repo.save(makePlaylist("Existing", "pl_1"));

    auto found = repo.findByName("Nonexistent");
    EXPECT_FALSE(found.has_value());
}

// ============================================================================
// searchByName
// ============================================================================

TEST_F(PlaylistRepositoryTest, SearchByNamePartialMatch)
{
    PlaylistRepository repo(m_storagePath);
    repo.save(makePlaylist("Rock Classics", "pl_1"));
    repo.save(makePlaylist("Jazz Favorites", "pl_2"));
    repo.save(makePlaylist("Rock Ballads", "pl_3"));

    auto results = repo.searchByName("rock");
    EXPECT_EQ(results.size(), 2);
}

TEST_F(PlaylistRepositoryTest, SearchByNameCaseInsensitive)
{
    PlaylistRepository repo(m_storagePath);
    repo.save(makePlaylist("Morning Chill", "pl_1"));

    auto results = repo.searchByName("MORNING");
    EXPECT_EQ(results.size(), 1);
}

TEST_F(PlaylistRepositoryTest, SearchByNameNoMatch)
{
    PlaylistRepository repo(m_storagePath);
    repo.save(makePlaylist("Workout", "pl_1"));

    auto results = repo.searchByName("classical");
    EXPECT_TRUE(results.empty());
}

TEST_F(PlaylistRepositoryTest, SearchByNameEmpty)
{
    PlaylistRepository repo(m_storagePath);
    auto results = repo.searchByName("anything");
    EXPECT_TRUE(results.empty());
}

// ============================================================================
// Serialize / Deserialize Round-Trip
// ============================================================================

TEST_F(PlaylistRepositoryTest, SaveAndLoadRoundTrip)
{
    {
        // Tạo repo, thêm dữ liệu, destructor sẽ saveToDisk
        PlaylistRepository repo(m_storagePath);
        auto playlist = makePlaylist("Round Trip", "pl_rt");
        auto media = makeMedia("song1.mp3");
        media.setTitle("Song Title");
        media.setArtist("Song Artist");
        playlist.addItem(media);
        playlist.addItem(makeMedia("song2.mp3"));
        repo.save(playlist);
    }

    // Tạo repo mới → constructor loadFromDisk
    PlaylistRepository repo2(m_storagePath);
    EXPECT_EQ(repo2.count(), 1);

    auto found = repo2.findById("pl_rt");
    ASSERT_TRUE(found.has_value());
    EXPECT_EQ(found->getName(), "Round Trip");
    // Items phải tồn tại thực trên đĩa để isValid() = true khi deserialize
    EXPECT_GE(found->getItemCount(), 1);
}

TEST_F(PlaylistRepositoryTest, SaveToDiskExplicit)
{
    PlaylistRepository repo(m_storagePath);
    repo.save(makePlaylist("Explicit Save", "pl_exp"));

    EXPECT_TRUE(repo.saveToDisk());
}

TEST_F(PlaylistRepositoryTest, LoadFromDiskExplicit)
{
    PlaylistRepository repo(m_storagePath);
    EXPECT_TRUE(repo.loadFromDisk());
}

// ============================================================================
// Edge Cases
// ============================================================================

TEST_F(PlaylistRepositoryTest, SaveOverwriteExisting)
{
    PlaylistRepository repo(m_storagePath);
    repo.save(makePlaylist("Version 1", "pl_ow"));
    repo.save(makePlaylist("Version 2", "pl_ow"));

    EXPECT_EQ(repo.count(), 1);
    auto found = repo.findById("pl_ow");
    ASSERT_TRUE(found.has_value());
    EXPECT_EQ(found->getName(), "Version 2");
}
