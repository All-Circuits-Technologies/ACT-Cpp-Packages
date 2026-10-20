// SPDX-FileCopyrightText: 2026 Benoit Rolandeau <benoit.rolandeau@allcircuits.com>
//
// SPDX-License-Identifier: LicenseRef-ALLCircuits-ACT-1.1

/**
 * @file test_file_util.cpp
 * @brief Unit tests of FileUtil.
 *
 * Covers reading and writing whole files (text and integers), creating ExtFile handles with or
 * without opening them, comparing paths and extracting file names. Every file lives in a
 * temporary directory removed after each test.
 */

#include "act_files/ext_file.hpp"
#include "act_files/file_util.hpp"
#include "act_test_common/temporary_directory.hpp"

#include <gtest/gtest.h>

#include <filesystem>
#include <optional>
#include <string>

namespace act::files::FileUtil
{
namespace
{

    using act::foundation::LogsLevel;

    class FileUtilTest : public act::tests::TemporaryDirectoryTest
    {
    };

    TEST_F(FileUtilTest, ReadFileGivesWhatWriteFileWrote)
    {
        const std::string path = getPath("content.txt");

        ASSERT_TRUE(WriteFile(path, std::string("first line\nsecond line\n"), getLogger()));

        EXPECT_EQ(ReadFile(path, getLogger()),
                  std::optional<std::string>("first line\nsecond line\n"));
    }

    TEST_F(FileUtilTest, ReadFileOfEmptyFileIsEmpty)
    {
        const std::string path = getPath("empty.txt");
        ASSERT_TRUE(WriteFile(path, std::string(), getLogger()));

        EXPECT_EQ(ReadFile(path, getLogger()), std::optional<std::string>(""));
    }

    TEST_F(FileUtilTest, ReadFileOfMissingFileFails)
    {
        EXPECT_EQ(ReadFile(getPath("missing.txt"), getLogger()), std::nullopt);
    }

    TEST_F(FileUtilTest, WriteFileReplacesTheContentByDefault)
    {
        const std::string path = getPath("replaced.txt");

        ASSERT_TRUE(WriteFile(path, std::string("old content"), getLogger()));
        ASSERT_TRUE(WriteFile(path, std::string("new"), getLogger()));

        EXPECT_EQ(ReadFile(path, getLogger()), std::optional<std::string>("new"));
    }

    TEST_F(FileUtilTest, WriteFileAppendsInAppendMode)
    {
        const std::string path = getPath("appended.txt");

        ASSERT_TRUE(WriteFile(path, std::string("first;"), getLogger()));
        ASSERT_TRUE(WriteFile(path, std::string("second;"), getLogger(), std::ios::app));

        EXPECT_EQ(ReadFile(path, getLogger()), std::optional<std::string>("first;second;"));
    }

    TEST_F(FileUtilTest, WriteFileWritesNumbersAsDecimalText)
    {
        const std::string intPath = getPath("int.txt");
        const std::string longPath = getPath("long.txt");

        ASSERT_TRUE(WriteFile(intPath, -42, getLogger()));
        ASSERT_TRUE(WriteFile(longPath, 1234567890123LL, getLogger()));

        EXPECT_EQ(ReadFile(intPath, getLogger()), std::optional<std::string>("-42"));
        EXPECT_EQ(ReadFile(longPath, getLogger()), std::optional<std::string>("1234567890123"));
    }

    TEST_F(FileUtilTest, WriteFileFailsInAMissingDirectory)
    {
        EXPECT_FALSE(WriteFile(getPath("missing/file.txt"), std::string("content"), getLogger()));
    }

    TEST_F(FileUtilTest, ReadFileAsIntParsesADecimalNumber)
    {
        const std::string path = getPath("number.txt");

        ASSERT_TRUE(WriteFile(path, std::string("1234"), getLogger()));
        EXPECT_EQ(ReadFileAsInt(path, getLogger()), std::optional<int>(1234));

        ASSERT_TRUE(WriteFile(path, std::string("-17\n"), getLogger()));
        EXPECT_EQ(ReadFileAsInt(path, getLogger()), std::optional<int>(-17));
    }

    TEST_F(FileUtilTest, ReadFileAsIntRejectsANonNumber)
    {
        const std::string path = getPath("text.txt");
        ASSERT_TRUE(WriteFile(path, std::string("not a number"), getLogger()));

        EXPECT_EQ(ReadFileAsInt(path, getLogger()), std::nullopt);
        EXPECT_EQ(getLogger().count(LogsLevel::ERR), 1U);
    }

    TEST_F(FileUtilTest, ReadFileAsIntAcceptsSurroundingWhitespace)
    {
        const std::string path = getPath("number.txt");
        ASSERT_TRUE(WriteFile(path, std::string(" \t42 \r\n"), getLogger()));

        EXPECT_EQ(ReadFileAsInt(path, getLogger()), std::optional<int>(42));
    }

    TEST_F(FileUtilTest, ReadFileAsIntSkipsAnUtf8ByteOrderMark)
    {
        const std::string path = getPath("number.txt");
        ASSERT_TRUE(WriteFile(path,
                              std::string("\xEF\xBB\xBF"
                                          "12"),
                              getLogger()));

        EXPECT_EQ(ReadFileAsInt(path, getLogger()), std::optional<int>(12));
    }

    TEST_F(FileUtilTest, ReadFileAsIntRejectsCharactersAfterTheNumber)
    {
        const std::string path = getPath("text.txt");
        ASSERT_TRUE(WriteFile(path, std::string("12abc"), getLogger()));

        EXPECT_EQ(ReadFileAsInt(path, getLogger()), std::nullopt);
    }

    TEST_F(FileUtilTest, ReadFileAsIntRejectsAnUtf16File)
    {
        using namespace std::string_literals;
        const std::string path = getPath("number.txt");

        // Little endian without byte order mark: the first digit used to be read alone
        ASSERT_TRUE(WriteFile(path,
                              "1\0"
                              "2\0"s,
                              getLogger()));
        EXPECT_EQ(ReadFileAsInt(path, getLogger()), std::nullopt);

        ASSERT_TRUE(WriteFile(path,
                              "\xFF\xFE"
                              "1\0"
                              "2\0"s,
                              getLogger()));
        EXPECT_EQ(ReadFileAsInt(path, getLogger()), std::nullopt);
    }

    TEST_F(FileUtilTest, ReadFileAsIntRejectsAnOutOfRangeNumber)
    {
        const std::string path = getPath("huge.txt");
        ASSERT_TRUE(WriteFile(path, std::string("99999999999999999999"), getLogger()));

        EXPECT_EQ(ReadFileAsInt(path, getLogger()), std::nullopt);
    }

    TEST_F(FileUtilTest, ReadFileAsIntOfMissingFileFails)
    {
        EXPECT_EQ(ReadFileAsInt(getPath("missing.txt"), getLogger()), std::nullopt);
    }

    TEST_F(FileUtilTest, CreateFileWithModeGivesAnOpenFile)
    {
        const std::string path = getPath("opened.txt");

        auto file = CreateFile(path, getLogger(), std::ios::out);

        ASSERT_NE(file, nullptr);
        EXPECT_TRUE(file->isOpen());
        EXPECT_EQ(file->getFilePath(), path);
        EXPECT_FALSE(file->isTemporary());
        EXPECT_TRUE(std::filesystem::exists(path));
    }

    TEST_F(FileUtilTest, CreateFileWithModeFailsWhenTheFileCannotBeOpened)
    {
        EXPECT_EQ(CreateFile(getPath("missing/opened.txt"), getLogger(), std::ios::out), nullptr);
        EXPECT_EQ(getLogger().count(LogsLevel::ERR), 1U);
    }

    TEST_F(FileUtilTest, CreateFileWithoutModeDoesNotOpenTheFile)
    {
        const std::string path = getPath("closed.txt");

        auto file = CreateFile(path, getLogger());

        ASSERT_NE(file, nullptr);
        EXPECT_FALSE(file->isOpen());
        EXPECT_EQ(file->getFilePath(), path);
        EXPECT_FALSE(std::filesystem::exists(path));
    }

    TEST_F(FileUtilTest, TemporaryFileIsDeletedWithItsLastHandle)
    {
        const std::string path = getPath("temporary.txt");

        auto file = CreateFile(path, getLogger(), std::ios::out, true);
        ASSERT_NE(file, nullptr);
        EXPECT_TRUE(file->isTemporary());
        EXPECT_TRUE(std::filesystem::exists(path));

        file.reset();

        EXPECT_FALSE(std::filesystem::exists(path));
    }

    TEST_F(FileUtilTest, ArePathsEqualAcceptsIdenticalStrings)
    {
        // Identical strings are equal without touching the file system, even when nothing exists
        EXPECT_TRUE(ArePathsEqual(getPath("missing.txt"), getPath("missing.txt"), getLogger()));
    }

    TEST_F(FileUtilTest, ArePathsEqualAcceptsTwoSpellingsOfTheSameFile)
    {
        const std::string path = getPath("same.txt");
        ASSERT_TRUE(WriteFile(path, std::string("content"), getLogger()));
        ASSERT_TRUE(std::filesystem::create_directory(getDirectory() / "sub"));
        const std::string otherSpelling =
            (getDirectory() / "." / "sub" / ".." / "same.txt").string();

        EXPECT_TRUE(ArePathsEqual(path, otherSpelling, getLogger()));
    }

    TEST_F(FileUtilTest, ArePathsEqualRejectsDifferentFiles)
    {
        const std::string first = getPath("first.txt");
        const std::string second = getPath("second.txt");
        ASSERT_TRUE(WriteFile(first, std::string("content"), getLogger()));
        ASSERT_TRUE(WriteFile(second, std::string("content"), getLogger()));

        EXPECT_FALSE(ArePathsEqual(first, second, getLogger()));
    }

    TEST_F(FileUtilTest, ArePathsEqualRejectsMissingFilesAndWarns)
    {
        EXPECT_FALSE(ArePathsEqual(getPath("missing.txt"), getPath("./missing.txt"), getLogger()));
        EXPECT_EQ(getLogger().count(LogsLevel::WARNING), 1U);
    }

    TEST(FileUtilFilenameTest, GetFilenameKeepsOnlyTheLastComponent)
    {
        EXPECT_EQ(GetFilename("/var/log/messages.log"), "messages.log");
        EXPECT_EQ(GetFilename("relative/dir/archive.tar.gz"), "archive.tar.gz");
        EXPECT_EQ(GetFilename("file.txt"), "file.txt");
    }

    TEST(FileUtilFilenameTest, GetFilenameIsEmptyForEmptyPathOrDirectoryPath)
    {
        EXPECT_EQ(GetFilename(""), "");
        EXPECT_EQ(GetFilename("dir/sub/"), "");
        EXPECT_EQ(GetFilename("/"), "");
    }

} // namespace
} // namespace act::files::FileUtil
