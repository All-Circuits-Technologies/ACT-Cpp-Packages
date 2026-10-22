// SPDX-FileCopyrightText: 2026 Anthony Loiseau <anthony.loiseau@allcircuits.com>
//
// SPDX-License-Identifier: LicenseRef-ALLCircuits-ACT-1.1

/**
 * @file test_archive_writer.cpp
 * @brief Unit tests of ArchiveWriter and StringArchiveSink.
 *
 * Covers writing ZIP archives in memory and reading them back: the empty archive, files and
 * directories, binary and empty contents, the per entry compression, nested archives, non ASCII
 * names under the "C" locale, the rejected paths, quoted in the logs with their control characters
 * escaped, the operations after finalizing, a failing or throwing sink and an abandoned writer;
 * the separators and the trailing separator of a directory.
 */

#include "act_archive/reader/archive_reader.hpp"
#include "act_archive/writer/abs_archive_sink.hpp"
#include "act_archive/writer/archive_writer.hpp"
#include "act_archive/writer/string_archive_sink.hpp"
#include "act_test_common/recording_logger.hpp"

#include <gtest/gtest.h>

#include <climits>
#include <clocale>
#include <locale.h>
#include <memory>
#include <optional>
#include <stdexcept>
#include <string>
#include <string_view>
#include <vector>

namespace act::archive
{
namespace
{

    using act::foundation::LogsLevel;
    using act::tests::RecordingLogger;

    /** @brief Size of an empty ZIP archive: its end of central directory record */
    constexpr std::size_t EMPTY_ZIP_SIZE = 22;

    /** @brief Size of the highly compressible content used to check the compression */
    constexpr std::size_t MEBIBYTE = 1024U * 1024U;

    /** @brief Signature of a ZIP local file header */
    const std::string LOCAL_HEADER_SIGNATURE("PK\x03\x04", 4);

    /** @brief Offset of the general purpose flags in a ZIP local file header */
    constexpr std::size_t LOCAL_HEADER_FLAGS_OFFSET = 6;

    /** @brief Signature of a ZIP central directory file header */
    const std::string CENTRAL_HEADER_SIGNATURE("PK\x01\x02", 4);

    /** @brief Offset of the general purpose flags in a ZIP central directory file header */
    constexpr std::size_t CENTRAL_HEADER_FLAGS_OFFSET = 8;

    /** @brief General purpose flag telling the entry name is in UTF-8 (bit 11) */
    constexpr unsigned int UTF8_NAME_FLAG = 0x0800;

    /** @brief Test if the first ZIP header with the given signature has the UTF-8 name flag */
    bool HasUtf8NameFlag(const std::string &zip, const std::string &signature, std::size_t offset)
    {
        const std::size_t header = zip.find(signature);
        if (header == std::string::npos || header + offset + 1 >= zip.size())
        {
            return false;
        }

        const auto low = static_cast<unsigned char>(zip[header + offset]);
        const auto high = static_cast<unsigned char>(zip[header + offset + 1]);
        return ((low | (high << CHAR_BIT)) & UTF8_NAME_FLAG) != 0;
    }

    /** @brief True if the calling thread uses the global locale, as when no locale is switched */
    bool IsThreadOnTheGlobalLocale()
    {
#if defined(_WIN32)
        return true;
#else
        return uselocale(static_cast<locale_t>(nullptr)) == LC_GLOBAL_LOCALE;
#endif
    }

    /** @brief Sink failing every write */
    class FailingSink : public AbsArchiveSink
    {
      public:
        [[nodiscard]] bool write(std::string_view /*chunk*/) override
        {
            return false;
        }
    };

    /** @brief Sink throwing on every write */
    class ThrowingSink : public AbsArchiveSink
    {
      public:
        [[nodiscard]] bool write(std::string_view /*chunk*/) override
        {
            throw std::runtime_error("disk full");
        }
    };

    /** @brief Read back a ZIP archive, expecting it to be valid */
    std::vector<ArchiveEntry> ReadZip(const std::string &data)
    {
        const RecordingLogger logger;
        std::unique_ptr<ArchiveReader> reader =
            ArchiveReader::OpenFromMemory(data, {.allowedFormats = {ArchiveFormat::ZIP}}, logger);
        EXPECT_NE(reader, nullptr);
        std::optional<std::vector<ArchiveEntry>> entries =
            (reader != nullptr) ? reader->readAllEntries() : std::nullopt;
        EXPECT_TRUE(entries.has_value());
        return entries.value_or(std::vector<ArchiveEntry>());
    }

    TEST(StringArchiveSinkTest, TakeDataGivesTheWrittenBytesAndEmptiesTheSink)
    {
        StringArchiveSink sink;

        EXPECT_TRUE(sink.write("ab"));
        EXPECT_TRUE(sink.write(std::string("\0c", 2)));

        EXPECT_EQ(sink.takeData(), std::string("ab\0c", 4));
        EXPECT_EQ(sink.takeData(), "");
    }

    TEST(ArchiveWriterTest, EmptyZipIsTheEndRecordOnlyAndReadsBackEmpty)
    {
        const RecordingLogger logger;
        StringArchiveSink sink;
        std::unique_ptr<ArchiveWriter> writer =
            ArchiveWriter::Create({.format = ArchiveFormat::ZIP}, sink, logger);
        ASSERT_NE(writer, nullptr);

        ASSERT_TRUE(writer->finalize());

        const std::string data = sink.takeData();
        EXPECT_EQ(data.size(), EMPTY_ZIP_SIZE);
        EXPECT_TRUE(ReadZip(data).empty());
    }

    TEST(ArchiveWriterTest, FilesAndDirectoriesReadBackInOrder)
    {
        const RecordingLogger logger;
        StringArchiveSink sink;
        std::unique_ptr<ArchiveWriter> writer =
            ArchiveWriter::Create({.format = ArchiveFormat::ZIP}, sink, logger);
        ASSERT_NE(writer, nullptr);
        const std::string binary("\0\x01\xFF\0", 4);

        ASSERT_TRUE(writer->addDirectoryEntry("My Folder"));
        ASSERT_TRUE(writer->addFileFromMemory("My Folder/script.js", "function main() {}\n"));
        ASSERT_TRUE(writer->addFileFromMemory("My Folder/empty.json", ""));
        ASSERT_TRUE(writer->addFileFromMemory("My Folder/sub/binary.bin", binary));
        ASSERT_TRUE(writer->finalize());

        const std::vector<ArchiveEntry> entries = ReadZip(sink.takeData());
        ASSERT_EQ(entries.size(), 4U);
        EXPECT_EQ(entries[0].path, "My Folder");
        EXPECT_EQ(entries[0].type, EntryType::DIRECTORY);
        EXPECT_EQ(entries[1].path, "My Folder/script.js");
        EXPECT_EQ(entries[1].type, EntryType::REGULAR_FILE);
        EXPECT_EQ(entries[1].content, "function main() {}\n");
        EXPECT_EQ(entries[2].path, "My Folder/empty.json");
        EXPECT_EQ(entries[2].content, "");
        EXPECT_EQ(entries[3].path, "My Folder/sub/binary.bin");
        EXPECT_EQ(entries[3].content, binary);
        EXPECT_EQ(logger.count(LogsLevel::WARNING) + logger.count(LogsLevel::ERR), 0U);
    }

    TEST(ArchiveWriterTest, DefaultCompressionDeflatesAndNoneStores)
    {
        const std::string zeros(MEBIBYTE, '\0');
        std::size_t compressedSize = 0;
        std::size_t storedSize = 0;

        for (const EntryCompression compression :
             {EntryCompression::DEFAULT, EntryCompression::NONE})
        {
            const RecordingLogger logger;
            StringArchiveSink sink;
            std::unique_ptr<ArchiveWriter> writer =
                ArchiveWriter::Create({.format = ArchiveFormat::ZIP}, sink, logger);
            ASSERT_NE(writer, nullptr);
            ASSERT_TRUE(
                writer->addFileFromMemory("zeros.bin", zeros, {.compression = compression}));
            ASSERT_TRUE(writer->finalize());

            const std::string data = sink.takeData();
            const std::vector<ArchiveEntry> entries = ReadZip(data);
            ASSERT_EQ(entries.size(), 1U);
            EXPECT_EQ(entries[0].content, zeros);
            (compression == EntryCompression::NONE ? storedSize : compressedSize) = data.size();
        }

        EXPECT_LT(compressedSize, MEBIBYTE / 100);
        EXPECT_GT(storedSize, MEBIBYTE);
    }

    TEST(ArchiveWriterTest, NestedArchiveReadsBackFromItsEntry)
    {
        const RecordingLogger logger;
        StringArchiveSink bundleSink;
        std::unique_ptr<ArchiveWriter> bundle =
            ArchiveWriter::Create({.format = ArchiveFormat::ZIP}, bundleSink, logger);
        ASSERT_NE(bundle, nullptr);
        ASSERT_TRUE(bundle->addDirectoryEntry("P"));
        ASSERT_TRUE(bundle->addFileFromMemory("P/data.csv", "a;b\n1;2\n"));
        ASSERT_TRUE(bundle->finalize());

        StringArchiveSink batchSink;
        std::unique_ptr<ArchiveWriter> batch =
            ArchiveWriter::Create({.format = ArchiveFormat::ZIP}, batchSink, logger);
        ASSERT_NE(batch, nullptr);
        ASSERT_TRUE(batch->addFileFromMemory("P.zip",
                                             bundleSink.takeData(),
                                             {.compression = EntryCompression::NONE}));
        ASSERT_TRUE(batch->finalize());

        const std::vector<ArchiveEntry> batchEntries = ReadZip(batchSink.takeData());
        ASSERT_EQ(batchEntries.size(), 1U);
        EXPECT_EQ(batchEntries[0].path, "P.zip");
        const std::vector<ArchiveEntry> bundleEntries = ReadZip(batchEntries[0].content);
        ASSERT_EQ(bundleEntries.size(), 2U);
        EXPECT_EQ(bundleEntries[1].path, "P/data.csv");
        EXPECT_EQ(bundleEntries[1].content, "a;b\n1;2\n");
    }

    TEST(ArchiveWriterTest, NonAsciiNamesSurviveTheCLocaleWhichStaysUntouched)
    {
        const std::string processLocale = std::setlocale(LC_ALL, nullptr);
        ASSERT_TRUE(IsThreadOnTheGlobalLocale());
        const std::string name =
            "Documents accentu\xC3\xA9s/R\xC3\xA9union \xC3\xA0 Gen\xC3\xA8ve.txt";
        const RecordingLogger logger;
        StringArchiveSink sink;
        std::unique_ptr<ArchiveWriter> writer =
            ArchiveWriter::Create({.format = ArchiveFormat::ZIP}, sink, logger);
        ASSERT_NE(writer, nullptr);

        ASSERT_TRUE(writer->addFileFromMemory(name, "x"));
        ASSERT_TRUE(writer->finalize());

        // The other tools read the name as UTF-8 thanks to the flag, not as an OEM code page
        const std::string data = sink.takeData();
        EXPECT_TRUE(HasUtf8NameFlag(data, LOCAL_HEADER_SIGNATURE, LOCAL_HEADER_FLAGS_OFFSET));
        EXPECT_TRUE(HasUtf8NameFlag(data, CENTRAL_HEADER_SIGNATURE, CENTRAL_HEADER_FLAGS_OFFSET));
        const std::vector<ArchiveEntry> entries = ReadZip(data);
        ASSERT_EQ(entries.size(), 1U);
        EXPECT_EQ(entries[0].path, name);
        EXPECT_EQ(std::setlocale(LC_ALL, nullptr), processLocale);
        EXPECT_TRUE(IsThreadOnTheGlobalLocale());
    }

    TEST(ArchiveWriterTest, NonAsciiDirectoryNamesAreFlaggedAsUtf8UnderTheCLocale)
    {
        const std::string processLocale = std::setlocale(LC_ALL, nullptr);
        ASSERT_TRUE(IsThreadOnTheGlobalLocale());
        const std::string name = "Dossier \xC3\xA9t\xC3\xA9";
        const RecordingLogger logger;
        StringArchiveSink sink;
        std::unique_ptr<ArchiveWriter> writer =
            ArchiveWriter::Create({.format = ArchiveFormat::ZIP}, sink, logger);
        ASSERT_NE(writer, nullptr);

        ASSERT_TRUE(writer->addDirectoryEntry(name));
        ASSERT_TRUE(writer->finalize());

        const std::string data = sink.takeData();
        EXPECT_TRUE(HasUtf8NameFlag(data, LOCAL_HEADER_SIGNATURE, LOCAL_HEADER_FLAGS_OFFSET));
        EXPECT_TRUE(HasUtf8NameFlag(data, CENTRAL_HEADER_SIGNATURE, CENTRAL_HEADER_FLAGS_OFFSET));
        const std::vector<ArchiveEntry> entries = ReadZip(data);
        ASSERT_EQ(entries.size(), 1U);
        EXPECT_EQ(entries[0].path, name);
        EXPECT_EQ(entries[0].type, EntryType::DIRECTORY);
        EXPECT_EQ(std::setlocale(LC_ALL, nullptr), processLocale);
        EXPECT_TRUE(IsThreadOnTheGlobalLocale());
    }

    TEST(ArchiveWriterTest, UnsafeOrDuplicatedPathsAreRejectedAndLeaveTheArchiveUntouched)
    {
        const RecordingLogger logger;
        StringArchiveSink sink;
        std::unique_ptr<ArchiveWriter> writer =
            ArchiveWriter::Create({.format = ArchiveFormat::ZIP}, sink, logger);
        ASSERT_NE(writer, nullptr);
        ASSERT_TRUE(writer->addFileFromMemory("kept.txt", "kept"));

        for (const std::string path :
             {"", "/abs.txt", "\\abs.txt", "../evil.txt", R"(a\..\..\evil.txt)", "C:x", "a//b"})
        {
            EXPECT_FALSE(writer->addFileFromMemory(path, "x")) << path;
            EXPECT_FALSE(writer->addDirectoryEntry(path)) << path;
        }
        EXPECT_FALSE(writer->addFileFromMemory("dir/", "x"));
        EXPECT_FALSE(writer->addFileFromMemory("kept.txt", "again"));
        EXPECT_FALSE(writer->addDirectoryEntry("kept.txt"));
        EXPECT_FALSE(writer->addDirectoryEntry("kept.txt/"));
        ASSERT_TRUE(writer->finalize());

        const std::vector<ArchiveEntry> entries = ReadZip(sink.takeData());
        ASSERT_EQ(entries.size(), 1U);
        EXPECT_EQ(entries[0].content, "kept");
        EXPECT_GT(logger.count(LogsLevel::ERR), 0U);
    }

    TEST(ArchiveWriterTest, EscapesTheControlCharactersOfTheLoggedPaths)
    {
        const RecordingLogger logger;
        StringArchiveSink sink;
        std::unique_ptr<ArchiveWriter> writer =
            ArchiveWriter::Create({.format = ArchiveFormat::ZIP}, sink, logger);
        ASSERT_NE(writer, nullptr);

        EXPECT_FALSE(writer->addFileFromMemory("evil\n[ERROR] forged line.txt", "x"));
        EXPECT_TRUE(
            logger.contains(LogsLevel::ERR, "'evil\\x0A[ERROR] forged line.txt' is not safe"));
        for (const RecordingLogger::Entry &entry : logger.getEntries())
        {
            EXPECT_EQ(entry.message.find('\n'), std::string::npos) << entry.message;
        }
    }

    TEST(ArchiveWriterTest, BackslashesAreWrittenAsSlashes)
    {
        const RecordingLogger logger;
        StringArchiveSink sink;
        std::unique_ptr<ArchiveWriter> writer =
            ArchiveWriter::Create({.format = ArchiveFormat::ZIP}, sink, logger);
        ASSERT_NE(writer, nullptr);

        ASSERT_TRUE(writer->addDirectoryEntry("dir\\sub"));
        ASSERT_TRUE(writer->addFileFromMemory("dir\\sub\\file.txt", "x"));
        EXPECT_FALSE(writer->addFileFromMemory("dir/sub/file.txt", "again"));
        ASSERT_TRUE(writer->finalize());

        const std::vector<ArchiveEntry> entries = ReadZip(sink.takeData());
        ASSERT_EQ(entries.size(), 2U);
        EXPECT_EQ(entries[0].path, "dir/sub");
        EXPECT_EQ(entries[1].path, "dir/sub/file.txt");
    }

    TEST(ArchiveWriterTest, DirectoryAcceptsATrailingSeparator)
    {
        const RecordingLogger logger;
        StringArchiveSink sink;
        std::unique_ptr<ArchiveWriter> writer =
            ArchiveWriter::Create({.format = ArchiveFormat::ZIP}, sink, logger);
        ASSERT_NE(writer, nullptr);

        ASSERT_TRUE(writer->addDirectoryEntry("first/"));
        ASSERT_TRUE(writer->addDirectoryEntry("second\\"));
        EXPECT_FALSE(writer->addDirectoryEntry("first"));
        ASSERT_TRUE(writer->finalize());

        const std::vector<ArchiveEntry> entries = ReadZip(sink.takeData());
        ASSERT_EQ(entries.size(), 2U);
        EXPECT_EQ(entries[0].path, "first");
        EXPECT_EQ(entries[1].path, "second");
    }

    TEST(ArchiveWriterTest, OperationsAfterFinalizingFail)
    {
        const RecordingLogger logger;
        StringArchiveSink sink;
        std::unique_ptr<ArchiveWriter> writer =
            ArchiveWriter::Create({.format = ArchiveFormat::ZIP}, sink, logger);
        ASSERT_NE(writer, nullptr);
        ASSERT_TRUE(writer->finalize());

        EXPECT_FALSE(writer->addFileFromMemory("late.txt", "x"));
        EXPECT_FALSE(writer->addDirectoryEntry("late"));
        EXPECT_FALSE(writer->finalize());
        EXPECT_EQ(sink.takeData().size(), EMPTY_ZIP_SIZE);
    }

    TEST(ArchiveWriterTest, FailingSinkMakesTheWriterFail)
    {
        const RecordingLogger logger;
        FailingSink sink;
        std::unique_ptr<ArchiveWriter> writer =
            ArchiveWriter::Create({.format = ArchiveFormat::ZIP}, sink, logger);
        ASSERT_NE(writer, nullptr);

        // libarchive buffers its output: the sink may only be called when finalizing
        const bool added = writer->addFileFromMemory("file.txt", "content");
        const bool finalized = added && writer->finalize();

        EXPECT_FALSE(finalized);
        EXPECT_FALSE(writer->addFileFromMemory("other.txt", "x"));
        EXPECT_FALSE(writer->finalize());
        EXPECT_GT(logger.count(LogsLevel::ERR), 0U);
    }

    TEST(ArchiveWriterTest, ThrowingSinkMakesTheWriterFailWithoutThrowing)
    {
        const RecordingLogger logger;
        ThrowingSink sink;
        std::unique_ptr<ArchiveWriter> writer =
            ArchiveWriter::Create({.format = ArchiveFormat::ZIP}, sink, logger);
        ASSERT_NE(writer, nullptr);

        // libarchive buffers its output: the sink may only be called when finalizing
        bool finalized = true;
        EXPECT_NO_THROW({
            const bool added = writer->addFileFromMemory("file.txt", "content");
            finalized = added && writer->finalize();
        });

        EXPECT_FALSE(finalized);
        EXPECT_TRUE(logger.contains(LogsLevel::ERR, "the archive sink threw: disk full"));
        EXPECT_FALSE(writer->addFileFromMemory("other.txt", "x"));
    }

    TEST(ArchiveWriterTest, AbandonedWriterDoesNotCompleteTheArchive)
    {
        const RecordingLogger logger;
        StringArchiveSink sink;
        {
            std::unique_ptr<ArchiveWriter> writer =
                ArchiveWriter::Create({.format = ArchiveFormat::ZIP}, sink, logger);
            ASSERT_NE(writer, nullptr);
            ASSERT_TRUE(writer->addFileFromMemory("file.txt", "content"));
        }

        const std::string data = sink.takeData();
        EXPECT_EQ(data.find("PK\x05\x06"), std::string::npos);
    }

} // namespace
} // namespace act::archive
