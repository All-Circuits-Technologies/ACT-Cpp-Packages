// SPDX-FileCopyrightText: 2026 Anthony Loiseau <anthony.loiseau@allcircuits.com>
//
// SPDX-License-Identifier: LicenseRef-ALLCircuits-ACT-1.1

#include "reader/archive_reader_impl.hpp"

#include "act_archive/utilities/archive_path_util.hpp"
#include "act_foundation/logger/abs_logger.hpp"
#include "utilities/libarchive_util.hpp"
#include "utilities/scoped_utf8_locale.hpp"

#include <archive.h>
#include <archive_entry.h>

#include <algorithm>
#include <array>
#include <climits>
#include <cstddef>
#include <cstdint>
#include <optional>
#include <set>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

namespace act::archive
{
ArchiveReader::Impl::Impl(const std::string &data,
                          ArchiveReaderOptions options,
                          const act::foundation::AbsLogger &logger)
    : m_data(data),
      m_options(std::move(options)),
      m_logger(logger)
{
}

ArchiveReader::Impl::~Impl()
{
    closeLibarchive();
}

bool ArchiveReader::Impl::open()
{
    if (m_options.allowedFormats.empty())
    {
        m_logger.warningStream() << "ArchiveReader: archive rejected: no archive format is allowed";
        return false;
    }

    if (!createLibarchiveReader())
    {
        m_logger.errorStream() << "ArchiveReader: cannot create the libarchive reader";
        return false;
    }

    // Open the archive: recognizes its format among the allowed ones, without decompressing yet
    if (archive_read_open_memory(m_archive, m_data.data(), m_data.size()) == ARCHIVE_OK)
    {
        return true;
    }

    // libarchive does not recognize an empty ZIP archive (an end record only): detect it here
    const bool isZipAllowed = std::ranges::find(m_options.allowedFormats, ArchiveFormat::ZIP) !=
                              m_options.allowedFormats.end();
    if (isZipAllowed && isEmptyZip())
    {
        m_isEmptyZip = true;
        return true;
    }

    m_logger.warningStream()
        << "ArchiveReader: archive rejected: unknown format or damaged archive ("
        << LibarchiveUtil::ErrorMessage(m_archive) << ")";
    return false;
}

std::optional<std::vector<ArchiveEntry>> ArchiveReader::Impl::readAllEntries()
{
    if (m_state != State::OPEN)
    {
        return reject("its entries were already read");
    }

    m_state = State::READ;
    std::optional<std::vector<ArchiveEntry>> entries = readEntries();

    // Entries contains uncompressed data, we can close the libarchive reader now
    closeLibarchive();
    return entries;
}

bool ArchiveReader::Impl::createLibarchiveReader()
{
    m_archive = archive_read_new();
    if (m_archive == nullptr)
    {
        return false;
    }

    // A new libarchive reader recognizes no format: enable the allowed ones only, one by one, and
    // never all of them, so that any other format is rejected (safer)
    for (const ArchiveFormat format : m_options.allowedFormats)
    {
        if (!LibarchiveUtil::EnableReaderFormat(m_archive, format))
        {
            m_logger.errorStream() << "ArchiveReader: cannot enable an allowed archive format ("
                                   << LibarchiveUtil::ErrorMessage(m_archive) << ")";
            return false;
        }
    }

    // No compression filter (gzip, zstd...) for now: only uncompressed archive data
    if (archive_read_support_filter_none(m_archive) != ARCHIVE_OK)
    {
        m_logger.errorStream() << "ArchiveReader: cannot enable the uncompressed data ("
                               << LibarchiveUtil::ErrorMessage(m_archive) << ")";
        return false;
    }

    return true;
}

void ArchiveReader::Impl::closeLibarchive()
{
    if (m_archive == nullptr)
    {
        return;
    }

    archive_read_free(m_archive);
    m_archive = nullptr;
}

std::optional<std::vector<ArchiveEntry>> ArchiveReader::Impl::readEntries()
{
    if (m_isEmptyZip)
    {
        return std::vector<ArchiveEntry>();
    }

    // libarchive converts the entry names according to the locale, when reading their headers:
    // switch this thread to UTF-8 meanwhile, so that non ASCII names survive a process running
    // under the "C" locale
    const ScopedUtf8Locale utf8Locale(m_logger);
    std::vector<ArchiveEntry> entries;
    while (true)
    {
        struct archive_entry *libarchiveEntry = nullptr;
        const int result = archive_read_next_header(m_archive, &libarchiveEntry);
        if (result == ARCHIVE_EOF)
        {
            break;
        }

        if (result != ARCHIVE_OK)
        {
            return reject("cannot read an entry header (" +
                          LibarchiveUtil::ErrorMessage(m_archive) + ")");
        }

        if (entries.size() >= m_options.limits.maxEntryCount)
        {
            return reject("more than " + std::to_string(m_options.limits.maxEntryCount) +
                          " entries");
        }

        std::optional<ArchiveEntry> entry = readHeader(libarchiveEntry);
        if (!entry.has_value())
        {
            return std::nullopt;
        }

        if (entry->type == EntryType::REGULAR_FILE)
        {
            std::optional<std::string> content = readContent(entry->path);
            if (!content.has_value())
            {
                return std::nullopt;
            }

            entry->content = std::move(content.value());
        }

        entries.push_back(std::move(entry.value()));
    }

    return entries;
}

bool ArchiveReader::Impl::isEmptyZip() const
{
    if (m_data.size() < ZIP_END_RECORD_SIZE || !m_data.starts_with(ZIP_END_RECORD_SIGNATURE))
    {
        return false;
    }

    const std::optional<std::uint16_t> commentLength =
        readLittleEndian16(ZIP_END_RECORD_COMMENT_LENGTH_OFFSET);
    return readLittleEndian16(ZIP_END_RECORD_ENTRIES_ON_DISK_OFFSET) == 0 &&
           readLittleEndian16(ZIP_END_RECORD_TOTAL_ENTRIES_OFFSET) == 0 &&
           commentLength.has_value() && m_data.size() == ZIP_END_RECORD_SIZE + *commentLength;
}

std::optional<std::uint16_t> ArchiveReader::Impl::readLittleEndian16(std::size_t offset) const
{
    if (m_data.size() < (offset + sizeof(std::uint16_t)))
    {
        return std::nullopt;
    }

    const auto low = static_cast<unsigned char>(m_data[offset]);
    const auto high = static_cast<unsigned char>(m_data[offset + 1]);
    return static_cast<std::uint16_t>(low | (high << CHAR_BIT));
}

std::optional<ArchiveEntry> ArchiveReader::Impl::readHeader(struct archive_entry *libarchiveEntry)
{
    const char *utf8Path = archive_entry_pathname_utf8(libarchiveEntry);
    if (utf8Path == nullptr)
    {
        return reject("an entry path cannot be decoded");
    }

    ArchiveEntry entry;
    entry.path = utf8Path;

    if (archive_entry_is_encrypted(libarchiveEntry) != 0)
    {
        return reject("the entry '" + entry.path + "' is encrypted");
    }

    if (archive_entry_hardlink(libarchiveEntry) != nullptr)
    {
        return reject("the entry '" + entry.path + "' is a hard link");
    }

    switch (archive_entry_filetype(libarchiveEntry))
    {
        case AE_IFREG:
            entry.type = EntryType::REGULAR_FILE;
            break;

        case AE_IFDIR:
            entry.type = EntryType::DIRECTORY;
            if (entry.path.ends_with(ArchivePathUtil::SEPARATOR) ||
                entry.path.ends_with(ArchivePathUtil::WINDOWS_SEPARATOR))
            {
                entry.path.pop_back();
            }
            break;

        default:
            return reject("the entry '" + entry.path + "' is neither a file nor a directory");
    }

    if (!ArchivePathUtil::IsSafeEntryPath(entry.path))
    {
        return reject("the entry path '" + entry.path + "' is not safe");
    }

    // "a/b" and "a\b" are the same file on Windows
    if (!m_paths.insert(ArchivePathUtil::NormalizeEntrySeparators(entry.path)).second)
    {
        return reject("the entry path '" + entry.path + "' is duplicated");
    }

    // Reject an oversized entry before decompressing it, on the size the archive announces;
    // readContent() checks the size again while decompressing, as an archive may lie
    if (entry.type == EntryType::REGULAR_FILE && archive_entry_size_is_set(libarchiveEntry) != 0)
    {
        const la_int64_t announcedSize = archive_entry_size(libarchiveEntry);
        const auto size = static_cast<std::size_t>(std::max<la_int64_t>(announcedSize, 0));
        const ArchiveReaderLimits &limits = m_options.limits;
        if (size > limits.maxEntrySize)
        {
            return reject("the entry '" + entry.path + "' exceeds " +
                          std::to_string(limits.maxEntrySize) + " bytes (announced size)");
        }

        if ((m_totalSize + size) > limits.maxTotalSize)
        {
            return reject("the entries exceed " + std::to_string(limits.maxTotalSize) +
                          " bytes together (announced size)");
        }
    }

    return entry;
}

std::optional<std::string> ArchiveReader::Impl::readContent(const std::string &path)
{
    const ArchiveReaderLimits &limits = m_options.limits;
    std::string content;
    std::array<char, READ_CHUNK_SIZE> buffer{};

    while (true)
    {
        const la_ssize_t readSize = archive_read_data(m_archive, buffer.data(), buffer.size());
        if (readSize == 0)
        {
            break;
        }

        if (readSize < 0)
        {
            return reject("cannot read the entry '" + path + "' (" +
                          LibarchiveUtil::ErrorMessage(m_archive) + ")");
        }

        const auto chunkSize = static_cast<std::size_t>(readSize);
        if ((content.size() + chunkSize) > limits.maxEntrySize)
        {
            return reject("the entry '" + path + "' exceeds " +
                          std::to_string(limits.maxEntrySize) + " bytes");
        }

        if ((m_totalSize + chunkSize) > limits.maxTotalSize)
        {
            return reject("the entries exceed " + std::to_string(limits.maxTotalSize) +
                          " bytes together");
        }

        content.append(buffer.data(), chunkSize);
        m_totalSize += chunkSize;
    }

    return content;
}

std::nullopt_t ArchiveReader::Impl::reject(const std::string &reason) const
{
    m_logger.warningStream() << "ArchiveReader: archive rejected: " << reason;
    return std::nullopt;
}
} // namespace act::archive
