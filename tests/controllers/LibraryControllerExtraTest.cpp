#include <gtest/gtest.h>
#include "controllers/LibraryController.h"
#include "models/LibraryModel.h"
#include "repositories/LibraryRepository.h"
#include "services/IMetadataReader.h"

using namespace media_player;

class DummyMetadataReader : public services::IMetadataReader {
public:
    std::unique_ptr<models::MetadataModel> readMetadata(const std::string& filePath) override { return nullptr; }
    bool canReadFile(const std::string& filePath) const override { return false; }
    bool writeMetadata(const std::string& filePath, const models::MetadataModel& metadata) override { return false; }
    bool extractCoverArt(const std::string& filePath, const std::string& outputPath) override { return false; }
    bool embedCoverArt(const std::string& filePath, const std::string& imagePath) override { return false; }
};

TEST(LibraryControllerExtraTest, SortSearchCountsAndFilters) {
    auto libraryModel = std::make_shared<models::LibraryModel>();
    auto libraryRepo = std::make_shared<repositories::LibraryRepository>("/tmp/lib_extra");
    auto metadataReader = std::make_shared<DummyMetadataReader>();
    controllers::LibraryController controller(libraryModel, libraryRepo, metadataReader);
    models::MediaFileModel a("/tmp/a.mp3"); a.setTitle("A"); a.setArtist("Alpha"); a.setAlbum("First");
    models::MediaFileModel b("/tmp/b.mp4"); b.setTitle("B"); b.setArtist("Beta"); b.setAlbum("Second");
    models::MediaFileModel c("/tmp/c.mp3"); c.setTitle("C"); c.setArtist("Gamma"); c.setAlbum("Third");
    libraryModel->addMedia(a);
    libraryModel->addMedia(b);
    libraryModel->addMedia(c);
    auto byTitleAsc = controller.sortByTitle(true);
    auto byArtistDesc = controller.sortByArtist(false);
    auto byAlbumAsc = controller.sortByAlbum(true);
    auto searchRes = controller.search("a");
    auto all = controller.getAllMedia();
    auto page = controller.getPage(0, 2);
    auto audioFiles = controller.getAudioFiles();
    auto videoFiles = controller.getVideoFiles();
    auto total = controller.getTotalCount();
    auto audioCount = controller.getAudioCount();
    auto videoCount = controller.getVideoCount();
    EXPECT_EQ(all.size(), 3u);
    EXPECT_EQ(total, 3u);
    EXPECT_EQ(audioCount, libraryModel->getTotalAudioFiles());
    EXPECT_EQ(videoCount, libraryModel->getTotalVideoFiles());
    EXPECT_FALSE(byTitleAsc.empty());
    EXPECT_FALSE(byArtistDesc.empty());
    EXPECT_FALSE(byAlbumAsc.empty());
    EXPECT_FALSE(searchRes.empty());
    EXPECT_EQ(page.size(), 2u);
    EXPECT_EQ(videoFiles.size(), 1u);
    EXPECT_EQ(audioFiles.size(), all.size());
}
