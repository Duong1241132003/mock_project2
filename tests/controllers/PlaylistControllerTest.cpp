#include <gtest/gtest.h>
#include <optional>
#include "controllers/PlaylistController.h"
#include "repositories/PlaylistRepository.h"
#include "models/MediaFileModel.h"

using namespace media_player;

// Use real repository for integration behavior

TEST(PlaylistControllerTest, CreateAndRenameAndDelete) {
    auto repo = std::make_shared<repositories::PlaylistRepository>("/tmp/pl_crud");
    controllers::PlaylistController controller(repo);
    EXPECT_FALSE(controller.createPlaylist(""));
    EXPECT_TRUE(controller.createPlaylist("a"));
    auto plOpt = controller.getPlaylistByName("a");
    ASSERT_TRUE(plOpt.has_value());
    auto id = plOpt->getId();
    EXPECT_TRUE(controller.renamePlaylist(id, "b"));
    EXPECT_TRUE(controller.deletePlaylist(id));
}

TEST(PlaylistControllerTest, AddRemoveMoveItems) {
    auto repo = std::make_shared<repositories::PlaylistRepository>("/tmp/pl_items");
    controllers::PlaylistController controller(repo);
    controller.createPlaylist("a");
    auto plOpt = controller.getPlaylistByName("a");
    ASSERT_TRUE(plOpt.has_value());
    auto id = plOpt->getId();
    models::MediaFileModel m1("/tmp/a.mp3");
    models::MediaFileModel m2("/tmp/b.mp3");
    EXPECT_TRUE(controller.addMediaToPlaylist(id, m1));
    EXPECT_TRUE(controller.addMediaToPlaylist(id, m2));
    auto items = controller.getPlaylistItems(id);
    ASSERT_EQ(items.size(), 2u);
    EXPECT_TRUE(controller.moveItemInPlaylist(id, 0, 1));
    EXPECT_TRUE(controller.removeMediaFromPlaylist(id, 1));
}

TEST(PlaylistControllerTest, NegativeBranchesAndCounts) {
    auto repo = std::make_shared<repositories::PlaylistRepository>("/tmp/pl_neg2");
    repo->clear();
    controllers::PlaylistController controller(repo);
    EXPECT_EQ(controller.getPlaylistCount(), 0u);
    EXPECT_FALSE(controller.renamePlaylist("none", "x"));
    EXPECT_FALSE(controller.deletePlaylist("none"));
    EXPECT_TRUE(controller.createPlaylist("c"));
    auto all = controller.getAllPlaylists();
    EXPECT_EQ(all.size(), 1u);
    auto plOpt = controller.getPlaylistByName("c");
    ASSERT_TRUE(plOpt.has_value());
    auto id = plOpt->getId();
    auto byId = controller.getPlaylistById(id);
    ASSERT_TRUE(byId.has_value());
    auto items = controller.getPlaylistItems("bad");
    EXPECT_TRUE(items.empty());
    EXPECT_FALSE(controller.addMediaToPlaylist("bad", models::MediaFileModel("/tmp/a.mp3")));
    EXPECT_FALSE(controller.removeMediaFromPlaylist("bad", 0));
    EXPECT_FALSE(controller.moveItemInPlaylist("bad", 0, 1));
    EXPECT_FALSE(controller.removeMediaFromPlaylist(id, 10));
    EXPECT_FALSE(controller.moveItemInPlaylist(id, 10, 11));
    EXPECT_FALSE(controller.createPlaylist("c"));
}
