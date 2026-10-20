// SPDX-FileCopyrightText: 2026 Benoit Rolandeau <benoit.rolandeau@allcircuits.com>
// SPDX-FileCopyrightText: 2026 Anthony Loiseau <anthony.loiseau@allcircuits.com>
//
// SPDX-License-Identifier: LicenseRef-ALLCircuits-ACT-1.1

/**
 * @file test_file_path_util.cpp
 * @brief Unit tests of FilePathUtil.
 *
 * Covers comparing paths and extracting file names. Every file lives in a temporary directory
 * removed after each test.
 */

#include "act_files/file_path_util.hpp"
#include "act_files/file_util.hpp"
#include "act_test_common/temporary_directory.hpp"

#include <gtest/gtest.h>

#include <filesystem>
#include <string>

namespace act::files::FilePathUtil
{
namespace
{

    using act::foundation::LogsLevel;

    class FilePathUtilTest : public act::tests::TemporaryDirectoryTest
    {
    };

    TEST_F(FilePathUtilTest, ArePathsEqualAcceptsIdenticalStrings)
    {
        // Identical strings are equal without touching the file system, even when nothing exists
        EXPECT_TRUE(ArePathsEqual(getPath("missing.txt"), getPath("missing.txt"), getLogger()));
    }

    TEST_F(FilePathUtilTest, ArePathsEqualAcceptsTwoSpellingsOfTheSameFile)
    {
        const std::string path = getPath("same.txt");
        ASSERT_TRUE(FileUtil::WriteFile(path, std::string("content"), getLogger()));
        ASSERT_TRUE(std::filesystem::create_directory(getDirectory() / "sub"));
        const std::string otherSpelling =
            (getDirectory() / "." / "sub" / ".." / "same.txt").string();

        EXPECT_TRUE(ArePathsEqual(path, otherSpelling, getLogger()));
    }

    TEST_F(FilePathUtilTest, ArePathsEqualResolvesRelativePathsAgainstTheCurrentDirectory)
    {
        const std::string path = getPath("relative.txt");
        ASSERT_TRUE(FileUtil::WriteFile(path, std::string("content"), getLogger()));
        const std::filesystem::path previousDirectory = std::filesystem::current_path();
        std::filesystem::current_path(getDirectory());

        const bool isEqual = ArePathsEqual("relative.txt", path, getLogger());

        std::filesystem::current_path(previousDirectory);
        EXPECT_TRUE(isEqual);
    }

    TEST_F(FilePathUtilTest, ArePathsEqualRejectsDifferentFiles)
    {
        const std::string first = getPath("first.txt");
        const std::string second = getPath("second.txt");
        ASSERT_TRUE(FileUtil::WriteFile(first, std::string("content"), getLogger()));
        ASSERT_TRUE(FileUtil::WriteFile(second, std::string("content"), getLogger()));

        EXPECT_FALSE(ArePathsEqual(first, second, getLogger()));
    }

    TEST_F(FilePathUtilTest, ArePathsEqualRejectsMissingFilesAndWarns)
    {
        EXPECT_FALSE(ArePathsEqual(getPath("missing.txt"), getPath("./missing.txt"), getLogger()));
        EXPECT_EQ(getLogger().count(LogsLevel::WARNING), 1U);
        EXPECT_TRUE(getLogger().contains(LogsLevel::WARNING, "ArePathsEqual"));
    }

    TEST(FilePathUtilFilenameTest, GetFilenameKeepsOnlyTheLastComponent)
    {
        EXPECT_EQ(GetFilename("/var/log/messages.log"), "messages.log");
        EXPECT_EQ(GetFilename("relative/dir/archive.tar.gz"), "archive.tar.gz");
        EXPECT_EQ(GetFilename("file.txt"), "file.txt");
    }

    TEST(FilePathUtilFilenameTest, GetFilenameIsEmptyForEmptyPathOrDirectoryPath)
    {
        EXPECT_EQ(GetFilename(""), "");
        EXPECT_EQ(GetFilename("dir/sub/"), "");
        EXPECT_EQ(GetFilename("/"), "");
    }

} // namespace
} // namespace act::files::FilePathUtil
