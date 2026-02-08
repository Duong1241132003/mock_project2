#include <gtest/gtest.h>
#include "controllers/LibraryController.h"
#include "models/LibraryModel.h"
#include "services/IMetadataReader.h"
#include "repositories/LibraryRepository.h"

using namespace media_player;

class FakeMetadataReader : public services::IMetadataReader {
public:
    bool canRead = true;
    bool writeOk = true;
    std::unique_ptr<models::MetadataModel> readMetadata(const std::string& filePath) override {
        if (!canRead) return nullptr;
        auto m = std::make_unique<models::MetadataModel>();
        m->setTitle("t");
        m->setArtist("a");
        m->setAlbum("al");
        return m;
    }
    bool canReadFile(const std::string& filePath) const override { return canRead; }
    bool writeMetadata(const std::string& filePath, const models::MetadataModel& metadata) override { return writeOk; }
    bool extractCoverArt(const std::string& filePath, const std::string& outputPath) override { return false; }
    bool embedCoverArt(const std::string& filePath, const std::string& imagePath) override { return false; }
};

class LibraryControllerTest : public testing::Test {
protected:
    void SetUp() override {
        libraryModel = std::make_shared<models::LibraryModel>();
        libraryRepo = std::make_shared<repositories::LibraryRepository>("/tmp/lib");
        metadataReader = std::make_shared<FakeMetadataReader>();
        controller = std::make_shared<controllers::LibraryController>(libraryModel, libraryRepo, metadataReader);
    }
    std::shared_ptr<models::LibraryModel> libraryModel;
    std::shared_ptr<repositories::LibraryRepository> libraryRepo;
    std::shared_ptr<FakeMetadataReader> metadataReader;
    std::shared_ptr<controllers::LibraryController> controller;
};

TEST_F(LibraryControllerTest, ReadMetadataWhenReadable) {
    auto res = controller->readMetadata("/tmp/a.mp3");
    EXPECT_TRUE(res.has_value());
    EXPECT_EQ(res->getTitle(), "t");
}

TEST_F(LibraryControllerTest, ReadMetadataNotReadable) {
    metadataReader->canRead = false;
    auto res = controller->readMetadata("/tmp/a.mp3");
    EXPECT_FALSE(res.has_value());
}

TEST_F(LibraryControllerTest, UpdateMetadataWritesAndUpdatesModel) {
    models::MediaFileModel media("/tmp/a.mp3");
    models::MetadataModel newMeta;
    newMeta.setTitle("nt");
    newMeta.setArtist("na");
    newMeta.setAlbum("nal");
    EXPECT_TRUE(controller->updateMetadata(media, newMeta));
}
