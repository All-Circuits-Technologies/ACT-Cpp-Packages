// SPDX-FileCopyrightText: 2026 Benoit Rolandeau <benoit.rolandeau@allcircuits.com>
//
// SPDX-License-Identifier: LicenseRef-ALLCircuits-ACT-1.1

/**
 * @file test_ext_file.cpp
 * @brief Unit tests of ExtFile.
 *
 * Covers the accessors, opening and closing, the deletion of temporary files on destruction, and
 * CreateFileAndTryToOpenIt. Every file lives in a temporary directory removed after each test.
 */

#include "act_files/ext_file.hpp"
#include "act_files/file_util.hpp"
#include "act_test_common/temporary_directory.hpp"

#include <gtest/gtest.h>

#include <filesystem>
#include <memory>
#include <optional>
#include <string>
#include <type_traits>

namespace act::files
{
namespace
{

    using act::foundation::LogsLevel;

    class ExtFileTest : public act::tests::TemporaryDirectoryTest
    {
    };

    TEST(ExtFileTypeTest, CanNeitherBeCopiedNorMoved)
    {
        // Checked at run time, so that a copiable class fails the test instead of the build. A
        // copy would share the owned file stream and delete it twice.
        EXPECT_FALSE(std::is_copy_constructible_v<ExtFile>);
        EXPECT_FALSE(std::is_copy_assignable_v<ExtFile>);
        EXPECT_FALSE(std::is_move_constructible_v<ExtFile>);
        EXPECT_FALSE(std::is_move_assignable_v<ExtFile>);
    }

    TEST_F(ExtFileTest, ConstructionDoesNotOpenNorCreateTheFile)
    {
        const std::string path = getPath("file.txt");

        ExtFile file(path, getLogger());

        EXPECT_FALSE(file.isOpen());
        EXPECT_FALSE(file.isTemporary());
        EXPECT_EQ(file.getFilePath(), path);
        EXPECT_FALSE(std::filesystem::exists(path));
    }

    TEST_F(ExtFileTest, AbsoluteFilePathResolvesARelativePath)
    {
        ExtFile file("relative/file.txt", getLogger());

        EXPECT_EQ(file.getAbsoluteFilePath(),
                  (std::filesystem::current_path() / "relative" / "file.txt").string());
    }

    TEST_F(ExtFileTest, OpenAndCloseUpdateTheOpenState)
    {
        ExtFile file(getPath("file.txt"), getLogger());

        ASSERT_TRUE(file.open(std::ios::out));
        EXPECT_TRUE(file.isOpen());

        file.close();
        EXPECT_FALSE(file.isOpen());
    }

    TEST_F(ExtFileTest, OpeningTwiceWithTheSameModeSucceeds)
    {
        ExtFile file(getPath("file.txt"), getLogger());

        ASSERT_TRUE(file.open(std::ios::out));
        EXPECT_TRUE(file.open(std::ios::out));
        EXPECT_TRUE(file.isOpen());
    }

    TEST_F(ExtFileTest, FileCanBeReopenedWithAnotherModeAfterClosing)
    {
        const std::string path = getPath("file.txt");
        ExtFile file(path, getLogger());

        ASSERT_TRUE(file.open(std::ios::out));
        file.accessFilePtr() << "written";
        file.close();

        ASSERT_TRUE(file.open(std::ios::in));
        std::string content;
        file.accessFilePtr() >> content;

        EXPECT_EQ(content, "written");
    }

    TEST_F(ExtFileTest, OpeningWithAnotherModeReopensTheFile)
    {
        const std::string path = getPath("file.txt");
        ExtFile file(path, getLogger());

        ASSERT_TRUE(file.open(std::ios::out));
        file.accessFilePtr() << "written";

        ASSERT_TRUE(file.open(std::ios::in));
        std::string content;
        file.accessFilePtr() >> content;

        EXPECT_EQ(content, "written");
        EXPECT_FALSE(file.accessFilePtr().fail());
    }

    TEST_F(ExtFileTest, OpenFailsWhenTheDirectoryIsMissing)
    {
        ExtFile file(getPath("missing/file.txt"), getLogger());

        EXPECT_FALSE(file.open(std::ios::out));
        EXPECT_FALSE(file.isOpen());
        EXPECT_EQ(getLogger().count(LogsLevel::ERR), 1U);
    }

    TEST_F(ExtFileTest, WrittenContentReachesTheFileOnDestruction)
    {
        const std::string path = getPath("file.txt");
        {
            ExtFile file(path, getLogger());
            ASSERT_TRUE(file.open(std::ios::out));
            file.accessFilePtr() << "content";
        }

        EXPECT_EQ(FileUtil::ReadFile(path, getLogger()), std::optional<std::string>("content"));
    }

    TEST_F(ExtFileTest, TemporaryFileIsDeletedOnDestruction)
    {
        const std::string path = getPath("temporary.txt");
        {
            ExtFile file(path, getLogger(), true);
            EXPECT_TRUE(file.isTemporary());
            ASSERT_TRUE(file.open(std::ios::out));
            EXPECT_TRUE(std::filesystem::exists(path));
        }

        EXPECT_FALSE(std::filesystem::exists(path));
        EXPECT_TRUE(getLogger().getEntries().empty());
    }

    TEST_F(ExtFileTest, TemporaryFileNeverCreatedWarnsOnDestruction)
    {
        const std::string path = getPath("never_created.txt");
        {
            ExtFile file(path, getLogger(), true);
        }

        EXPECT_TRUE(getLogger().contains(LogsLevel::WARNING, path));
    }

    TEST_F(ExtFileTest, CreateFileAndTryToOpenItGivesAnOpenFile)
    {
        const std::string path = getPath("file.txt");

        std::unique_ptr<ExtFile> file(
            ExtFile::CreateFileAndTryToOpenIt(path, std::ios::out, getLogger()));

        ASSERT_NE(file, nullptr);
        EXPECT_TRUE(file->isOpen());
        EXPECT_EQ(file->getFilePath(), path);
        EXPECT_TRUE(std::filesystem::exists(path));
    }

    TEST_F(ExtFileTest, CreateFileAndTryToOpenItFailsWhenTheFileCannotBeOpened)
    {
        std::unique_ptr<ExtFile> file(ExtFile::CreateFileAndTryToOpenIt(getPath("missing/file.txt"),
                                                                        std::ios::in,
                                                                        getLogger()));

        EXPECT_EQ(file, nullptr);
        EXPECT_EQ(getLogger().count(LogsLevel::ERR), 1U);
    }

    TEST_F(ExtFileTest, CreatedFileKeepsItsModeAcrossOpenCalls)
    {
        std::unique_ptr<ExtFile> file(
            ExtFile::CreateFileAndTryToOpenIt(getPath("file.txt"), std::ios::out, getLogger()));
        ASSERT_NE(file, nullptr);

        EXPECT_TRUE(file->open(std::ios::out));
        EXPECT_TRUE(file->isOpen());
    }

} // namespace
} // namespace act::files
