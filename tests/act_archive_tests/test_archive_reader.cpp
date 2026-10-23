// SPDX-FileCopyrightText: 2026 Anthony Loiseau <anthony.loiseau@allcircuits.com>
//
// SPDX-License-Identifier: LicenseRef-ALLCircuits-ACT-1.1

/**
 * @file test_archive_reader.cpp
 * @brief Unit tests of ArchiveReader.
 *
 * Covers opening and reading, once only, the ZIP archives of other tools (7-Zip, Windows 11,
 * Info-ZIP, Python), nested ones included, under the "C" locale; the empty archive; the paths given
 * with `/` separators, and quoted in the logs with their control characters escaped; the data
 * which cannot be a temporary; and every reason to reject an archive: no allowed format, other
 * format, damaged data, unsafe or duplicated paths, links, encrypted entries, the entry count,
 * entry size (zip bomb, on its announced size or while decompressing) and total size (on the
 * announced sizes or while decompressing) limits, and an entry whose size differs from the
 * announced one.
 */

#include "act_archive/reader/archive_reader.hpp"
#include "act_test_common/recording_logger.hpp"
#include "raw_archive_builder.hpp"
#include "zip_fixtures.hpp"

#include <gtest/gtest.h>

#include <clocale>
#include <locale.h>
#include <memory>
#include <optional>
#include <string>
#include <utility>
#include <vector>

namespace act::archive
{
namespace
{

    using act::foundation::LogsLevel;
    using act::tests::RecordingLogger;
    using tests::RawArchiveBuilder;
    namespace ZipFixtures = tests::ZipFixtures;

    /** @brief Size of the highly compressible content used to check the limits */
    constexpr std::size_t MEBIBYTE = 1024U * 1024U;

    /** @brief Size limit far below MEBIBYTE */
    constexpr std::size_t KIBIBYTE = 1024U;

    /** @brief Name of the directory most fixtures hold */
    const std::string FIXTURE_DIRECTORY = "Documents accentu\xC3\xA9s";

    /** @brief Size of a ZIP end of central directory record without comment */
    constexpr std::size_t END_RECORD_SIZE = 22;

    /** @brief Options allowing ZIP archives only, with the default limits */
    const ArchiveReaderOptions ZIP_ONLY{.allowedFormats = {ArchiveFormat::ZIP}};

    /** @brief Open an archive and read all its entries, std::nullopt if any step fails */
    std::optional<std::vector<ArchiveEntry>> Read(const std::string &data,
                                                  const ArchiveReaderOptions &options,
                                                  const RecordingLogger &logger)
    {
        std::unique_ptr<ArchiveReader> reader =
            ArchiveReader::OpenFromMemory(data, options, logger);
        if (reader == nullptr)
        {
            return std::nullopt;
        }

        return reader->readAllEntries();
    }

    /** @brief Same as above, with a logger the test does not check */
    std::optional<std::vector<ArchiveEntry>> Read(const std::string &data,
                                                  const ArchiveReaderOptions &options = ZIP_ONLY)
    {
        const RecordingLogger logger;
        return Read(data, options, logger);
    }

    /**
     * @brief True if ArchiveReader::OpenFromMemory accepts data of this type
     * @note The reader keeps a reference to the data: a temporary must not compile
     */
    template <typename Data>
    concept CanOpenFromMemory = requires(Data &&data, const RecordingLogger &logger) {
        ArchiveReader::OpenFromMemory(std::forward<Data>(data), ZIP_ONLY, logger);
    };

    static_assert(CanOpenFromMemory<const std::string &>);
    static_assert(CanOpenFromMemory<std::string &>);
    static_assert(!CanOpenFromMemory<std::string>);
    static_assert(!CanOpenFromMemory<const char *>);

    /** @brief Check the entries of the fixtures holding the accented directory */
    void ExpectAccentedDirectory(const std::vector<ArchiveEntry> &entries)
    {
        ASSERT_EQ(entries.size(), 4U);
        EXPECT_EQ(entries[0].path, FIXTURE_DIRECTORY);
        EXPECT_EQ(entries[0].type, EntryType::DIRECTORY);
        EXPECT_EQ(entries[1].path, FIXTURE_DIRECTORY + "/Ordre du jour.txt");
        EXPECT_EQ(entries[1].content, "Point 1 : budget");
        EXPECT_EQ(entries[2].path,
                  FIXTURE_DIRECTORY + "/R\xC3\xA9union \xC3\xA0 Gen\xC3\xA8ve.txt");
        EXPECT_EQ(entries[2].content,
                  "Caf\xC3\xA9 \xC3\xA0 No\xC3\xABl, d\xC3\xA9j\xC3\xA0 pr\xC3\xA9vu");
        EXPECT_EQ(entries[3].path, FIXTURE_DIRECTORY + "/Vide (parenth\xC3\xA8ses).txt");
        EXPECT_EQ(entries[3].type, EntryType::REGULAR_FILE);
        EXPECT_EQ(entries[3].content, "");
    }

    TEST(ArchiveReaderTest, ReadsSevenZipArchiveWithOemNamesUnderTheCLocale)
    {
        const std::string processLocale = std::setlocale(LC_ALL, nullptr);
#if !defined(_WIN32)
        // The reading switches the locale of the calling thread only, then restores it
        ASSERT_EQ(uselocale(static_cast<locale_t>(nullptr)), LC_GLOBAL_LOCALE);
#endif

        const auto entries = Read(ZipFixtures::ToString(ZipFixtures::SEVEN_ZIP_WINDOWS_BYTES));

        ASSERT_TRUE(entries.has_value());
        ExpectAccentedDirectory(entries.value());
        EXPECT_EQ(std::setlocale(LC_ALL, nullptr), processLocale);
#if !defined(_WIN32)
        EXPECT_EQ(uselocale(static_cast<locale_t>(nullptr)), LC_GLOBAL_LOCALE);
#endif
    }

    TEST(ArchiveReaderTest, ReadsWindows11Archive)
    {
        const auto entries = Read(ZipFixtures::ToString(ZipFixtures::WINDOWS_11_SEND_TO_BYTES));

        ASSERT_TRUE(entries.has_value());
        ExpectAccentedDirectory(entries.value());
    }

    TEST(ArchiveReaderTest, ReadsInfoZipArchive)
    {
        const auto entries = Read(ZipFixtures::ToString(ZipFixtures::INFO_ZIP_BYTES));

        ASSERT_TRUE(entries.has_value());
        ExpectAccentedDirectory(entries.value());
    }

    TEST(ArchiveReaderTest, ReadsPythonZipfileArchive)
    {
        const auto entries = Read(ZipFixtures::ToString(ZipFixtures::PYTHON_ZIPFILE_BYTES));

        ASSERT_TRUE(entries.has_value());
        ASSERT_EQ(entries->size(), 3U);
        EXPECT_EQ(entries->at(0).path, "Dossier \xC3\xA9t\xC3\xA9");
        EXPECT_EQ(entries->at(0).type, EntryType::DIRECTORY);
        EXPECT_EQ(entries->at(1).content, "Name;Value\n\xC3\x89t\xC3\xA9;A\n");
        EXPECT_EQ(entries->at(2).path, "Dossier \xC3\xA9t\xC3\xA9/script.js");
        EXPECT_EQ(entries->at(2).content, "function main() {}\n");
    }

    TEST(ArchiveReaderTest, ReadsNestedWindows11Archives)
    {
        const auto outer = Read(ZipFixtures::ToString(ZipFixtures::WINDOWS_11_NESTED_BYTES));

        ASSERT_TRUE(outer.has_value());
        ASSERT_EQ(outer->size(), 2U);
        for (const ArchiveEntry &inner : outer.value())
        {
            EXPECT_TRUE(inner.path.ends_with(".zip")) << inner.path;
            const auto innerEntries = Read(inner.content);
            ASSERT_TRUE(innerEntries.has_value()) << inner.path;
            ExpectAccentedDirectory(innerEntries.value());
        }
    }

    TEST(ArchiveReaderTest, ReadsEmptyZipArchives)
    {
        const std::string endRecord("PK\x05\x06\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0",
                                    END_RECORD_SIZE);
        const std::string endRecordWithComment =
            endRecord.substr(0, END_RECORD_SIZE - 2) + std::string("\x03\0", 2) + "abc";

        const auto entries = Read(endRecord);
        const auto entriesWithComment = Read(endRecordWithComment);

        ASSERT_TRUE(entries.has_value());
        EXPECT_TRUE(entries->empty());
        ASSERT_TRUE(entriesWithComment.has_value());
        EXPECT_TRUE(entriesWithComment->empty());
    }

    TEST(ArchiveReaderTest, RejectsAnEndRecordAnnouncingEntries)
    {
        std::string endRecord("PK\x05\x06\0\0\0\0\x01\0\x01\0\0\0\0\0\0\0\0\0\0\0",
                              END_RECORD_SIZE);

        EXPECT_FALSE(Read(endRecord).has_value());
    }

    TEST(ArchiveReaderTest, RejectsWhenNoFormatIsAllowed)
    {
        const std::string data = ZipFixtures::ToString(ZipFixtures::INFO_ZIP_BYTES);
        const RecordingLogger logger;

        EXPECT_TRUE(ArchiveReader::OpenFromMemory(data, {}, logger) == nullptr);
        EXPECT_TRUE(logger.contains(LogsLevel::WARNING, "no archive format"));
    }

    TEST(ArchiveReaderTest, ReadsTheEntriesOnceOnly)
    {
        const std::string data = ZipFixtures::ToString(ZipFixtures::INFO_ZIP_BYTES);
        const RecordingLogger logger;
        std::unique_ptr<ArchiveReader> reader =
            ArchiveReader::OpenFromMemory(data, ZIP_ONLY, logger);
        ASSERT_NE(reader, nullptr);

        EXPECT_TRUE(reader->readAllEntries().has_value());
        EXPECT_FALSE(reader->readAllEntries().has_value());
        EXPECT_TRUE(logger.contains(LogsLevel::WARNING, "already read"));
    }

    TEST(ArchiveReaderTest, RejectedReadingCannotBeRetried)
    {
        const std::string zip = RawArchiveBuilder().addFile("1", "1").addFile("2", "2").build();
        ArchiveReaderOptions options = ZIP_ONLY;
        options.limits.maxEntryCount = 1;
        const RecordingLogger logger;
        std::unique_ptr<ArchiveReader> reader = ArchiveReader::OpenFromMemory(zip, options, logger);
        ASSERT_NE(reader, nullptr);

        EXPECT_FALSE(reader->readAllEntries().has_value());
        EXPECT_FALSE(reader->readAllEntries().has_value());
        EXPECT_TRUE(logger.contains(LogsLevel::WARNING, "already read"));
    }

    TEST(ArchiveReaderTest, OpenFailsOnAnUnrecognizedArchive)
    {
        const std::string tar =
            RawArchiveBuilder(RawArchiveBuilder::Format::TAR).addFile("a.txt", "a").build();
        const std::string garbage(300, 'x');
        const RecordingLogger logger;

        EXPECT_EQ(ArchiveReader::OpenFromMemory(tar, ZIP_ONLY, logger), nullptr);
        EXPECT_EQ(ArchiveReader::OpenFromMemory(garbage, ZIP_ONLY, logger), nullptr);
        EXPECT_TRUE(logger.contains(LogsLevel::WARNING, "unknown format or damaged archive"));
    }

    TEST(ArchiveReaderTest, OptionsAreCopiedWhenOpening)
    {
        const std::string data = ZipFixtures::ToString(ZipFixtures::INFO_ZIP_BYTES);
        const RecordingLogger logger;
        std::unique_ptr<ArchiveReader> reader;
        {
            ArchiveReaderOptions options = ZIP_ONLY;
            options.limits.maxEntryCount = 1;
            reader = ArchiveReader::OpenFromMemory(data, options, logger);
        }
        ASSERT_NE(reader, nullptr);

        // The limit given when opening still applies, the options being gone
        EXPECT_FALSE(reader->readAllEntries().has_value());
    }

    TEST(ArchiveReaderTest, RejectsAnotherFormatThanTheAllowedOnes)
    {
        const std::string tar =
            RawArchiveBuilder(RawArchiveBuilder::Format::TAR).addFile("a.txt", "a").build();

        EXPECT_FALSE(Read(tar).has_value());
    }

    TEST(ArchiveReaderTest, RejectsDamagedData)
    {
        const std::string valid = ZipFixtures::ToString(ZipFixtures::INFO_ZIP_BYTES);

        EXPECT_FALSE(Read("").has_value());
        EXPECT_FALSE(Read(std::string(300, 'x')).has_value());
        EXPECT_FALSE(Read(valid.substr(0, valid.size() / 2)).has_value());
        EXPECT_FALSE(Read(valid.substr(0, valid.size() - 10)).has_value());
    }

    TEST(ArchiveReaderTest, RejectsUnsafePaths)
    {
        for (const std::string path : {"../evil.txt",
                                       "..\\evil.txt",
                                       R"(a\..\..\evil.txt)",
                                       "/abs.txt",
                                       "C:x.txt",
                                       "./dot.txt",
                                       "a//b.txt"})
        {
            const std::string zip = RawArchiveBuilder().addFile(path, "x").build();
            const RecordingLogger logger;

            EXPECT_FALSE(Read(zip, ZIP_ONLY, logger).has_value()) << path;
            EXPECT_TRUE(logger.contains(LogsLevel::WARNING, "not safe")) << path;
        }
    }

    TEST(ArchiveReaderTest, EscapesTheControlCharactersOfTheLoggedPaths)
    {
        const std::string zip =
            RawArchiveBuilder().addFile("evil\n[ERROR] forged line.txt", "x").build();
        const RecordingLogger logger;

        EXPECT_FALSE(Read(zip, ZIP_ONLY, logger).has_value());
        EXPECT_TRUE(
            logger.contains(LogsLevel::WARNING, "'evil\\x0A[ERROR] forged line.txt' is not safe"));
        for (const RecordingLogger::Entry &entry : logger.getEntries())
        {
            EXPECT_EQ(entry.message.find('\n'), std::string::npos) << entry.message;
        }
    }

    TEST(ArchiveReaderTest, GivesThePathsWithSlashSeparators)
    {
        // libarchive writes the directory as dir\sub/
        const std::string zip =
            RawArchiveBuilder().addDirectory("dir\\sub").addFile("dir\\sub\\a.txt", "a").build();

        const auto entries = Read(zip);

        ASSERT_TRUE(entries.has_value());
        ASSERT_EQ(entries->size(), 2U);
        EXPECT_EQ(entries->at(0).path, "dir/sub");
        EXPECT_EQ(entries->at(0).type, EntryType::DIRECTORY);
        EXPECT_EQ(entries->at(1).path, "dir/sub/a.txt");
        EXPECT_EQ(entries->at(1).content, "a");
    }

    TEST(ArchiveReaderTest, RejectsDuplicatedPaths)
    {
        const std::string zip =
            RawArchiveBuilder().addFile("dup.txt", "1").addFile("dup.txt", "2").build();

        EXPECT_FALSE(Read(zip).has_value());
    }

    TEST(ArchiveReaderTest, RejectsPathsDifferingOnlyByTheirSeparator)
    {
        const std::string zip =
            RawArchiveBuilder().addFile("a/b.txt", "1").addFile("a\\b.txt", "2").build();

        EXPECT_FALSE(Read(zip).has_value());
    }

    TEST(ArchiveReaderTest, RejectsADirectoryAndAFileWithTheSamePath)
    {
        const std::string zip =
            RawArchiveBuilder().addDirectory("same").addFile("same", "x").build();

        EXPECT_FALSE(Read(zip).has_value());
    }

    TEST(ArchiveReaderTest, RejectsSymbolicLinks)
    {
        const std::string zip = RawArchiveBuilder().addSymlink("link", "/etc/passwd").build();
        const RecordingLogger logger;

        EXPECT_FALSE(Read(zip, ZIP_ONLY, logger).has_value());
        EXPECT_TRUE(logger.contains(LogsLevel::WARNING, "neither a file nor a directory"));
    }

    TEST(ArchiveReaderTest, RejectsEncryptedEntries)
    {
        const std::string zip = ZipFixtures::ToString(ZipFixtures::ENCRYPTED_BYTES);
        const RecordingLogger logger;

        EXPECT_FALSE(Read(zip, ZIP_ONLY, logger).has_value());
        EXPECT_TRUE(logger.contains(LogsLevel::WARNING, "'secret.txt' is encrypted"));
    }

    TEST(ArchiveReaderTest, RejectsTooManyEntries)
    {
        const std::string zip =
            RawArchiveBuilder().addFile("1", "1").addFile("2", "2").addFile("3", "3").build();
        ArchiveReaderOptions options = ZIP_ONLY;

        options.limits.maxEntryCount = 3;
        EXPECT_TRUE(Read(zip, options).has_value());
        options.limits.maxEntryCount = 2;
        EXPECT_FALSE(Read(zip, options).has_value());
    }

    TEST(ArchiveReaderTest, RejectsAZipBombOnItsAnnouncedSizeBeforeDecompressing)
    {
        const std::string zip =
            RawArchiveBuilder().addFile("zeros.bin", std::string(MEBIBYTE, '\0')).build();
        ArchiveReaderOptions options = ZIP_ONLY;
        options.limits.maxEntrySize = KIBIBYTE;
        const RecordingLogger logger;

        ASSERT_LT(zip.size(), 2 * KIBIBYTE);
        EXPECT_FALSE(Read(zip, options, logger).has_value());
        EXPECT_TRUE(logger.contains(LogsLevel::WARNING, "exceeds 1024 bytes (announced size)"));
    }

    TEST(ArchiveReaderTest, RejectsAnUnderstatedEntryWhileDecompressing)
    {
        // The entry announces 100 bytes, under the limit, but holds 1 MiB
        const std::string zip = ZipFixtures::ToString(ZipFixtures::UNDERSTATED_SIZE_BYTES);
        ArchiveReaderOptions options = ZIP_ONLY;
        options.limits.maxEntrySize = KIBIBYTE;
        const RecordingLogger logger;

        EXPECT_FALSE(Read(zip, options, logger).has_value());
        EXPECT_TRUE(logger.contains(LogsLevel::WARNING, "exceeds 1024 bytes"));
        EXPECT_FALSE(logger.contains(LogsLevel::WARNING, "announced size"));
    }

    TEST(ArchiveReaderTest, RejectsAnEntryWhoseSizeDiffersFromTheAnnouncedOne)
    {
        const std::string zip = ZipFixtures::ToString(ZipFixtures::UNDERSTATED_SIZE_BYTES);
        const RecordingLogger logger;

        EXPECT_FALSE(Read(zip, ZIP_ONLY, logger).has_value());
        EXPECT_TRUE(logger.contains(LogsLevel::WARNING, "wrong size"));
        ASSERT_FALSE(logger.getEntries().empty());
        // The libarchive message ends with a line feed, removed
        EXPECT_FALSE(logger.getEntries().back().message.ends_with("\n)"));
    }

    TEST(ArchiveReaderTest, RejectsEntriesExceedingTheTotalSize)
    {
        const std::string content(KIBIBYTE, 'a');
        const std::string zip =
            RawArchiveBuilder().addFile("1", content).addFile("2", content).build();
        ArchiveReaderOptions options = ZIP_ONLY;

        const RecordingLogger logger;

        options.limits.maxTotalSize = 2 * KIBIBYTE;
        EXPECT_TRUE(Read(zip, options).has_value());
        options.limits.maxTotalSize = 2 * KIBIBYTE - 1;
        EXPECT_FALSE(Read(zip, options, logger).has_value());
        EXPECT_TRUE(logger.contains(LogsLevel::WARNING, "bytes together (announced size)"));
    }

    TEST(ArchiveReaderTest, RejectsEntriesExceedingTheTotalSizeWhileDecompressing)
    {
        // The entry announces 100 bytes, under both limits, but holds 1 MiB: only the total size
        // limit, far below the entry size limit, is exceeded while decompressing
        const std::string zip = ZipFixtures::ToString(ZipFixtures::UNDERSTATED_SIZE_BYTES);
        ArchiveReaderOptions options = ZIP_ONLY;
        options.limits.maxEntrySize = 2 * MEBIBYTE;
        options.limits.maxTotalSize = KIBIBYTE;
        const RecordingLogger logger;

        EXPECT_FALSE(Read(zip, options, logger).has_value());
        EXPECT_TRUE(logger.contains(LogsLevel::WARNING, "exceed 1024 bytes together"));
        EXPECT_FALSE(logger.contains(LogsLevel::WARNING, "announced size"));
    }

} // namespace
} // namespace act::archive
