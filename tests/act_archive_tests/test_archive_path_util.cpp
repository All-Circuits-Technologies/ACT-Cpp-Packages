// SPDX-FileCopyrightText: 2026 Anthony Loiseau <anthony.loiseau@allcircuits.com>
//
// SPDX-License-Identifier: LicenseRef-ALLCircuits-ACT-1.1

/**
 * @file test_archive_path_util.cpp
 * @brief Unit tests of ArchivePathUtil.
 *
 * Covers the entry paths accepted as safe (relative, UTF-8, punctuation, backslash separators)
 * and every reason to reject one: empty, absolute, colon, control characters, empty, `.` and `..`
 * components, with either separator; and the separator normalization.
 */

#include "act_archive/utilities/archive_path_util.hpp"

#include <gtest/gtest.h>

#include <string>

namespace act::archive::ArchivePathUtil
{
namespace
{

    TEST(ArchivePathUtilTest, IsSafeEntryPathAcceptsRelativePaths)
    {
        for (const std::string path : {"file.txt",
                                       "dir/file.txt",
                                       "a/b/c/d",
                                       "My Folder (v2)/data.csv",
                                       ".hidden/..name/...",
                                       // "données/Résumé.txt", in octal not to merge the "e"
                                       "donn\303\251es/R\303\251sum\303\251.txt"})
        {
            EXPECT_TRUE(IsSafeEntryPath(path)) << path;
        }
    }

    TEST(ArchivePathUtilTest, IsSafeEntryPathRejectsEmptyAndAbsolutePaths)
    {
        EXPECT_FALSE(IsSafeEntryPath(""));
        EXPECT_FALSE(IsSafeEntryPath("/"));
        EXPECT_FALSE(IsSafeEntryPath("/etc/passwd"));
    }

    TEST(ArchivePathUtilTest, IsSafeEntryPathTakesBackslashAsASeparator)
    {
        EXPECT_TRUE(IsSafeEntryPath("dir\\file.txt"));
        EXPECT_TRUE(IsSafeEntryPath("a\\b/c"));
        EXPECT_FALSE(IsSafeEntryPath("\\abs.txt"));
        EXPECT_FALSE(IsSafeEntryPath("..\\evil.txt"));
        EXPECT_FALSE(IsSafeEntryPath(R"(a\..\..\evil.txt)"));
        EXPECT_FALSE(IsSafeEntryPath("dir\\"));
        EXPECT_FALSE(IsSafeEntryPath("a\\\\b"));
    }

    TEST(ArchivePathUtilTest, IsSafeEntryPathRejectsWindowsDrivesAndStreams)
    {
        EXPECT_FALSE(IsSafeEntryPath("C:evil.txt"));
        EXPECT_FALSE(IsSafeEntryPath("C:/evil.txt"));
        EXPECT_FALSE(IsSafeEntryPath("file.txt:stream"));
    }

    TEST(ArchivePathUtilTest, IsSafeEntryPathRejectsControlCharacters)
    {
        EXPECT_FALSE(IsSafeEntryPath(std::string("a\0b", 3)));
        EXPECT_FALSE(IsSafeEntryPath("a\nb"));
        EXPECT_FALSE(IsSafeEntryPath("a\x1F"
                                     "b"));
        EXPECT_FALSE(IsSafeEntryPath("a\x7F"
                                     "b"));
    }

    TEST(ArchivePathUtilTest, IsSafeEntryPathRejectsDotAndEmptyComponents)
    {
        for (const std::string path : {".",
                                       "..",
                                       "../evil.txt",
                                       "dir/../../evil.txt",
                                       "dir/..",
                                       "./file.txt",
                                       "dir/./file.txt",
                                       "dir//file.txt",
                                       "dir/"})
        {
            EXPECT_FALSE(IsSafeEntryPath(path)) << path;
        }
    }

    TEST(ArchivePathUtilTest, NormalizeEntrySeparatorsReplacesEveryBackslash)
    {
        EXPECT_EQ(NormalizeEntrySeparators("a\\b/c\\d"), "a/b/c/d");
        EXPECT_EQ(NormalizeEntrySeparators("plain"), "plain");
        EXPECT_EQ(NormalizeEntrySeparators(""), "");
    }

} // namespace
} // namespace act::archive::ArchivePathUtil
