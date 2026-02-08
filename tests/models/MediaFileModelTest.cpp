#include <gtest/gtest.h>
#include "models/MediaFileModel.h"

using namespace media_player;

TEST(MediaFileModelTest, DetermineMediaTypeAudio) {
    models::MediaFileModel file("/home/user/music/song.mp3");
    EXPECT_EQ(file.getType(), models::MediaType::AUDIO);
}

TEST(MediaFileModelTest, DetermineMediaTypeVideo) {
    models::MediaFileModel file("/home/user/video/movie.mp4");
    EXPECT_EQ(file.getType(), models::MediaType::VIDEO);
}

TEST(MediaFileModelTest, CheckCaseSensitivity) {
    models::MediaFileModel file1("song.wav");
    EXPECT_EQ(file1.getType(), models::MediaType::AUDIO);
    
    models::MediaFileModel file2("SONG.WAV");
    // As per recent changes, .WAV is UNSUPPORTED
    EXPECT_EQ(file2.getType(), models::MediaType::UNSUPPORTED); 
}

TEST(MediaFileModelTest, SetGetProperties) {
    models::MediaFileModel file;
    file.setTitle("Test Title");
    file.setArtist("Test Artist");
    file.setDuration(120);
    
    EXPECT_EQ(file.getTitle(), "Test Title");
    EXPECT_EQ(file.getArtist(), "Test Artist");
    EXPECT_EQ(file.getDuration(), 120);
}
