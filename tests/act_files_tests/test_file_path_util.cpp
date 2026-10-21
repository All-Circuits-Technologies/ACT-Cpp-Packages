// SPDX-FileCopyrightText: 2026 Benoit Rolandeau <benoit.rolandeau@allcircuits.com>
// SPDX-FileCopyrightText: 2026 Anthony Loiseau <anthony.loiseau@allcircuits.com>
//
// SPDX-License-Identifier: LicenseRef-ALLCircuits-ACT-1.1

/**
 * @file test_file_path_util.cpp
 * @brief Unit tests of FilePathUtil.
 *
 * Covers comparing paths (files living in a temporary directory removed after each test),
 * extracting file names, detecting Windows reserved names, and sanitizing file names: allowed and
 * forbidden characters, ends of the name, hidden files, empty results, Windows reserved names,
 * truncation, of the fallback name too, and the options, valid or not.
 */

#include "act_files/file_path_util.hpp"
#include "act_files/file_util.hpp"
#include "act_test_common/recording_logger.hpp"
#include "act_test_common/temporary_directory.hpp"

#include <gtest/gtest.h>

#include <filesystem>
#include <string>

namespace act::files::FilePathUtil
{
namespace
{

    using act::foundation::LogsLevel;
    using act::tests::RecordingLogger;

    class FilePathUtilTest : public act::tests::TemporaryDirectoryTest
    {
    };

    /** @brief Sanitize with the only character set, with a logger the test does not check */
    std::string Sanitize(const std::string &name, const SanitizeFileNameOptions &options = {})
    {
        const RecordingLogger logger;
        return SanitizeFileName(name, FileNameCharset::WINDOWS_PRINTABLE_ASCII, logger, options);
    }

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

    TEST(FilePathUtilReservedNameTest, IsWindowsReservedNameAcceptsDeviceNames)
    {
        for (const std::string name : {"CON",
                                       "PRN",
                                       "AUX",
                                       "NUL",
                                       "CONIN$",
                                       "CONOUT$",
                                       "COM0",
                                       "COM9",
                                       "COM\xC2\xB9",
                                       "COM\xC2\xB3",
                                       "LPT0",
                                       "LPT9",
                                       "LPT\xC2\xB2"})
        {
            EXPECT_TRUE(IsWindowsReservedName(name)) << name;
        }
    }

    TEST(FilePathUtilReservedNameTest, IsWindowsReservedNameIgnoresCaseExtensionAndEndingSpaces)
    {
        EXPECT_TRUE(IsWindowsReservedName("con"));
        EXPECT_TRUE(IsWindowsReservedName("Nul.txt"));
        EXPECT_TRUE(IsWindowsReservedName("lpt1.tar.gz"));
        EXPECT_TRUE(IsWindowsReservedName("con .txt"));
        EXPECT_TRUE(IsWindowsReservedName("aux."));
        EXPECT_TRUE(IsWindowsReservedName("conout$.log"));
        EXPECT_TRUE(IsWindowsReservedName("com\xC2\xB9.txt"));
    }

    TEST(FilePathUtilReservedNameTest, IsWindowsReservedNameRejectsOtherNames)
    {
        for (const std::string name : {"",
                                       ".",
                                       "..",
                                       ".con",
                                       "console",
                                       "acon",
                                       "my.con",
                                       "com",
                                       "com10",
                                       "comx",
                                       "lpt",
                                       "conin",
                                       "com\xC2\xB4"})
        {
            EXPECT_FALSE(IsWindowsReservedName(name)) << name;
        }
    }

    TEST(FilePathUtilSanitizeTest, SanitizeFileNameKeepsAValidName)
    {
        EXPECT_EQ(Sanitize("My-File_1.v2"), "My-File_1.v2");
    }

    TEST(FilePathUtilSanitizeTest, SanitizeFileNameKeepsSpacesAndAllowedPunctuation)
    {
        EXPECT_EQ(Sanitize("Report #3 & summary (v2) [50%] {a,b;c} 'x' =+@^~!$`"),
                  "Report #3 & summary (v2) [50%] {a,b;c} 'x' =+@^~!$`");
    }

    TEST(FilePathUtilSanitizeTest, SanitizeFileNameReplacesEachForbiddenCharacter)
    {
        for (const char forbidden : std::string("<>:\"/\\|?*"))
        {
            EXPECT_EQ(Sanitize(std::string("a") + forbidden + "b"), "a_b") << forbidden;
        }
    }

    TEST(FilePathUtilSanitizeTest, SanitizeFileNameReplacesControlCharacters)
    {
        EXPECT_EQ(Sanitize(std::string("a\0b", 3)), "a_b");
        // \037 is the unit separator and \177 the delete character
        EXPECT_EQ(Sanitize("a\tb\nc\037d\177e"), "a_b_c_d_e");
    }

    TEST(FilePathUtilSanitizeTest, SanitizeFileNameMergesARunOfForbiddenCharacters)
    {
        EXPECT_EQ(Sanitize("a/\\|b"), "a_b");
        EXPECT_EQ(Sanitize("a / b"), "a _ b");
    }

    TEST(FilePathUtilSanitizeTest, SanitizeFileNameReplacesNonAsciiCharacters)
    {
        // Both bytes of each accented letter are replaced by a single character
        EXPECT_EQ(Sanitize("R\xC3\xA9sum\xC3\xA9 annuel (v2)"), "R_sum_ annuel (v2)");
        EXPECT_EQ(Sanitize("R\xC3\xA9union \xC3\xA0 Gen\xC3\xA8ve.txt"), "R_union _ Gen_ve.txt");
    }

    TEST(FilePathUtilSanitizeTest, SanitizeFileNameTrimsSpacesAndTheEndOnly)
    {
        EXPECT_EQ(Sanitize("  .hidden. "), ".hidden");
        EXPECT_EQ(Sanitize(" name "), "name");
        EXPECT_EQ(Sanitize("/name/"), "_name");
        EXPECT_EQ(Sanitize("name. . ."), "name");
        EXPECT_EQ(Sanitize("a..b"), "a..b");
    }

    TEST(FilePathUtilSanitizeTest, SanitizeFileNameKeepsTheStartOfHiddenAndSpecialFiles)
    {
        EXPECT_EQ(Sanitize(".bashrc"), ".bashrc");
        EXPECT_EQ(Sanitize("__init__.py"), "__init__.py");
        EXPECT_EQ(Sanitize("..name"), "..name");
    }

    TEST(FilePathUtilSanitizeTest, SanitizeFileNameFallsBackWhenNothingRemains)
    {
        EXPECT_EQ(Sanitize(""), "unnamed");
        EXPECT_EQ(Sanitize("."), "unnamed");
        EXPECT_EQ(Sanitize(".."), "unnamed");
        EXPECT_EQ(Sanitize("..."), "unnamed");
        EXPECT_EQ(Sanitize("__"), "unnamed");
        EXPECT_EQ(Sanitize("   "), "unnamed");
        EXPECT_EQ(Sanitize("\xC3\xA9"), "unnamed");
    }

    TEST(FilePathUtilSanitizeTest, SanitizeFileNameSuffixesWindowsReservedNames)
    {
        EXPECT_EQ(Sanitize("con"), "con_");
        EXPECT_EQ(Sanitize("CON"), "CON_");
        EXPECT_EQ(Sanitize("Nul.txt"), "Nul_.txt");
        EXPECT_EQ(Sanitize("prn"), "prn_");
        EXPECT_EQ(Sanitize("aux.log"), "aux_.log");
        EXPECT_EQ(Sanitize("com1"), "com1_");
        EXPECT_EQ(Sanitize("COM0"), "COM0_");
        EXPECT_EQ(Sanitize("lpt9.tar.gz"), "lpt9_.tar.gz");
        EXPECT_EQ(Sanitize("con .txt"), "con_ .txt");
        EXPECT_EQ(Sanitize("CONIN$"), "CONIN$_");
    }

    TEST(FilePathUtilSanitizeTest, SanitizeFileNameKeepsNamesMerelyLookingReserved)
    {
        EXPECT_EQ(Sanitize("console"), "console");
        EXPECT_EQ(Sanitize("com10"), "com10");
        EXPECT_EQ(Sanitize("comx"), "comx");
        EXPECT_EQ(Sanitize("acon"), "acon");
        EXPECT_EQ(Sanitize("my.con"), "my.con");
        EXPECT_EQ(Sanitize("conin"), "conin");
    }

    TEST(FilePathUtilSanitizeTest, SanitizeFileNameTruncatesToTheMaximumLength)
    {
        const std::string longName(100, 'a');

        EXPECT_EQ(Sanitize(longName), longName);
        EXPECT_EQ(Sanitize(longName, {.maxLength = 64}), std::string(64, 'a'));
        EXPECT_EQ(Sanitize(std::string(300, 'a')).size(),
                  SanitizeFileNameOptions::DEFAULT_MAX_LENGTH);
    }

    TEST(FilePathUtilSanitizeTest, SanitizeFileNameTrimsTheEndAgainAfterTruncation)
    {
        // The cut falls right after a dot, a space or a replacement character
        EXPECT_EQ(Sanitize("abc.def", {.maxLength = 4}), "abc");
        EXPECT_EQ(Sanitize("abc def", {.maxLength = 4}), "abc");
        EXPECT_EQ(Sanitize("abc/def", {.maxLength = 4}), "abc");
    }

    TEST(FilePathUtilSanitizeTest, SanitizeFileNameNeverRecreatesAReservedNameWhenTruncating)
    {
        EXPECT_EQ(Sanitize("con", {.maxLength = 3}), "co_");
        EXPECT_EQ(Sanitize("console", {.maxLength = 3}), "co_");
        EXPECT_EQ(Sanitize("com1.txt", {.maxLength = 4}), "com_");
        EXPECT_EQ(Sanitize("con.txt", {.maxLength = 7}), "co_.txt");
    }

    TEST(FilePathUtilSanitizeTest, SanitizeFileNameHonoursCustomOptions)
    {
        EXPECT_EQ(Sanitize("a/b", {.replacementChar = '-'}), "a-b");
        EXPECT_EQ(Sanitize("-a-", {.replacementChar = '-'}), "-a");
        EXPECT_EQ(Sanitize("con", {.replacementChar = '-'}), "con-");
        EXPECT_EQ(Sanitize("...", {.fallbackName = "fallback"}), "fallback");
    }

    TEST(FilePathUtilSanitizeTest, SanitizeFileNameTruncatesTheFallbackName)
    {
        EXPECT_EQ(Sanitize("...", {.maxLength = 3}), "unn");
    }

    TEST(FilePathUtilSanitizeTest, SanitizeFileNameFallsBackWhenTheReplacementKeepsItReserved)
    {
        EXPECT_EQ(
            Sanitize("COM1", {.replacementChar = '0', .maxLength = 4, .fallbackName = "file"}),
            "file");
        EXPECT_EQ(Sanitize("CONIN$.txt",
                           {.replacementChar = '$', .maxLength = 10, .fallbackName = "file"}),
                  "file");
        EXPECT_EQ(Sanitize("COM1", {.replacementChar = '0', .maxLength = 5}), "COM10");
    }

    TEST(FilePathUtilSanitizeTest, SanitizeFileNameDoesNotWarnWithValidOptions)
    {
        const RecordingLogger logger;

        EXPECT_EQ(SanitizeFileName("a/b",
                                   FileNameCharset::WINDOWS_PRINTABLE_ASCII,
                                   logger,
                                   {.replacementChar = '-', .maxLength = 64}),
                  "a-b");
        EXPECT_EQ(logger.count(LogsLevel::WARNING), 0U);
    }

    TEST(FilePathUtilSanitizeTest, SanitizeFileNameReplacesAnInvalidReplacementCharacterAndWarns)
    {
        for (const char invalid : std::string(". /*\xC3"))
        {
            const RecordingLogger logger;

            EXPECT_EQ(SanitizeFileName("a<b",
                                       FileNameCharset::WINDOWS_PRINTABLE_ASCII,
                                       logger,
                                       {.replacementChar = invalid}),
                      "a_b")
                << static_cast<int>(invalid);
            EXPECT_EQ(logger.count(LogsLevel::WARNING), 1U) << static_cast<int>(invalid);
            EXPECT_TRUE(
                logger.contains(LogsLevel::WARNING,
                                "(code " + std::to_string(static_cast<unsigned char>(invalid))))
                << static_cast<int>(invalid);
        }
    }

    TEST(FilePathUtilSanitizeTest, SanitizeFileNameQuotesAPrintableInvalidReplacementCharacter)
    {
        const RecordingLogger logger;

        EXPECT_EQ(SanitizeFileName("a<b",
                                   FileNameCharset::WINDOWS_PRINTABLE_ASCII,
                                   logger,
                                   {.replacementChar = '*'}),
                  "a_b");
        EXPECT_TRUE(logger.contains(LogsLevel::WARNING, "'*' (code 42)"));
    }

    TEST(FilePathUtilSanitizeTest, SanitizeFileNameHandlesAZeroMaximumLengthAsOneAndWarns)
    {
        const RecordingLogger logger;

        EXPECT_EQ(SanitizeFileName("abc",
                                   FileNameCharset::WINDOWS_PRINTABLE_ASCII,
                                   logger,
                                   {.maxLength = 0}),
                  "a");
        EXPECT_EQ(logger.count(LogsLevel::WARNING), 1U);
    }

} // namespace
} // namespace act::files::FilePathUtil
