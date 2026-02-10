/**
 * @file AppConfigTest.cpp
 * @brief Unit tests cho AppConfig — singleton cấu hình ứng dụng
 *
 * Bao gồm: singleton pattern, constants, supported formats.
 */

#include <gtest/gtest.h>
#include "config/AppConfig.h"

#include <algorithm>

using namespace media_player::config;

// ============================================================================
// Singleton Pattern
// ============================================================================

TEST(AppConfigTest, GetInstanceReturnsSameInstance)
{
    auto& instance1 = AppConfig::getInstance();
    auto& instance2 = AppConfig::getInstance();

    // Cùng một địa chỉ bộ nhớ → cùng instance
    EXPECT_EQ(&instance1, &instance2);
}

// ============================================================================
// Static Constants — Supported Formats
// ============================================================================

TEST(AppConfigTest, SupportedAudioExtensionsNotEmpty)
{
    EXPECT_FALSE(AppConfig::SUPPORTED_AUDIO_EXTENSIONS.empty());
}

TEST(AppConfigTest, SupportedAudioExtensionsContainsMp3)
{
    const auto& exts = AppConfig::SUPPORTED_AUDIO_EXTENSIONS;
    auto it = std::find(exts.begin(), exts.end(), ".mp3");
    EXPECT_NE(it, exts.end());
}

TEST(AppConfigTest, SupportedAudioExtensionsContainsWav)
{
    const auto& exts = AppConfig::SUPPORTED_AUDIO_EXTENSIONS;
    auto it = std::find(exts.begin(), exts.end(), ".wav");
    EXPECT_NE(it, exts.end());
}

TEST(AppConfigTest, ScannableExtensionsNotEmpty)
{
    EXPECT_FALSE(AppConfig::SCANNABLE_EXTENSIONS.empty());
}

TEST(AppConfigTest, ScannableExtensionsContainsAllSupported)
{
    // Tất cả supported audio extensions phải nằm trong scannable
    const auto& supported = AppConfig::SUPPORTED_AUDIO_EXTENSIONS;
    const auto& scannable = AppConfig::SCANNABLE_EXTENSIONS;

    for (const auto& ext : supported)
    {
        auto it = std::find(scannable.begin(), scannable.end(), ext);
        EXPECT_NE(it, scannable.end()) << ext << " not found in scannable extensions";
    }
}

TEST(AppConfigTest, ScannableExtensionsContainsFlac)
{
    const auto& exts = AppConfig::SCANNABLE_EXTENSIONS;
    auto it = std::find(exts.begin(), exts.end(), ".flac");
    EXPECT_NE(it, exts.end());
}

// ============================================================================
// Static Constants — Paths
// ============================================================================

TEST(AppConfigTest, PlaylistStoragePathNotEmpty)
{
    EXPECT_FALSE(AppConfig::PLAYLIST_STORAGE_PATH.empty());
}

TEST(AppConfigTest, LibraryStoragePathNotEmpty)
{
    EXPECT_FALSE(AppConfig::LIBRARY_STORAGE_PATH.empty());
}

TEST(AppConfigTest, HistoryStoragePathNotEmpty)
{
    EXPECT_FALSE(AppConfig::HISTORY_STORAGE_PATH.empty());
}

TEST(AppConfigTest, DefaultScanPathNotEmpty)
{
    EXPECT_FALSE(AppConfig::DEFAULT_SCAN_PATH.empty());
}

TEST(AppConfigTest, SerialPortDefaultNotEmpty)
{
    EXPECT_FALSE(AppConfig::SERIAL_PORT_DEFAULT.empty());
}

// ============================================================================
// Static Constants — Numeric Values
// ============================================================================

TEST(AppConfigTest, MaxItemsPerPagePositive)
{
    EXPECT_GT(AppConfig::MAX_ITEMS_PER_PAGE, 0);
}

TEST(AppConfigTest, MaxScanDepthPositive)
{
    EXPECT_GT(AppConfig::MAX_SCAN_DEPTH, 0);
}

TEST(AppConfigTest, DefaultVolumeInRange)
{
    EXPECT_GE(AppConfig::DEFAULT_VOLUME, 0);
    EXPECT_LE(AppConfig::DEFAULT_VOLUME, 100);
}

TEST(AppConfigTest, SerialBaudRatePositive)
{
    EXPECT_GT(AppConfig::SERIAL_BAUD_RATE, 0);
}

TEST(AppConfigTest, PlaybackUpdateIntervalPositive)
{
    EXPECT_GT(AppConfig::PLAYBACK_UPDATE_INTERVAL_MS, 0);
}
