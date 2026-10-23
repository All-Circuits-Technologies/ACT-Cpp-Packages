// SPDX-FileCopyrightText: 2026 Anthony Loiseau <anthony.loiseau@allcircuits.com>
//
// SPDX-License-Identifier: LicenseRef-ALLCircuits-ACT-1.1

#pragma once

#include "act_archive/reader/archive_reader.hpp"
#include "act_archive/types/archive_entry.hpp"
#include "act_foundation/not_copiable_not_movable.hpp"

#include <cstddef>
#include <cstdint>
#include <optional>
#include <set>
#include <string>
#include <string_view>
#include <vector>

struct archive;
struct archive_entry;

namespace act::foundation
{
class AbsLogger;
} // namespace act::foundation

namespace act::archive
{
/**
 * @brief Implementation of ArchiveReader: a single reading of an archive held in memory,
 *        wrapping a libarchive reader, freed by the destructor (RAII)
 */
class ArchiveReader::Impl : private act::foundation::NotCopiableNotMovable
{
  private:
    /** @brief States of the reader */
    enum class State
    {
        /** @brief The archive is opened, its entries not read yet */
        OPEN,

        /** @brief The entries were read, or their reading failed */
        READ,
    };

  public:
    /**
     * @brief Prepare the reading, see open()
     * @param data The archive bytes; they must outlive the reading
     * @param options The reading options, copied
     * @param logger The logger to use; it must outlive the reading
     */
    Impl(const std::string &data,
         ArchiveReaderOptions options,
         const act::foundation::AbsLogger &logger);

    /** @brief Free the libarchive reader, see closeLibarchive() */
    ~Impl() override;

  public:
    /**
     * @brief Check that a format is allowed, then recognize the archive: open the libarchive
     *        reader on the data, or detect an empty ZIP archive
     * @return True on success, false otherwise (the reason is logged)
     */
    [[nodiscard]] bool open();

    /** @copydoc ArchiveReader::readAllEntries */
    [[nodiscard]] std::optional<std::vector<ArchiveEntry>> readAllEntries();

  private:
    /**
     * @brief Create the libarchive reader, and enable the allowed formats only
     * @return True on success, false otherwise
     */
    [[nodiscard]] bool createLibarchiveReader();

    /**
     * @brief Close and free the libarchive reader, if any
     * @note Called twice, it does nothing the second time.
     */
    void closeLibarchive();

    /**
     * @brief Read all the entries, for readAllEntries()
     * @return The entries, or std::nullopt if the archive is rejected
     */
    [[nodiscard]] std::optional<std::vector<ArchiveEntry>> readEntries();

    /**
     * @brief Test if the data is an empty ZIP archive, which libarchive does not recognize
     * @details An empty ZIP archive is a single end of central directory record (22 bytes plus
     *          an optional comment), announcing no entry. Other formats need no such test.
     * @return True if the data is an empty ZIP archive, false otherwise
     */
    [[nodiscard]] bool isEmptyZip() const;

    /**
     * @brief Read a 16 bits little endian integer of the data
     * @param offset The offset of the integer in the data
     * @return The integer, or std::nullopt if its 2 bytes are not all in the data
     */
    [[nodiscard]] std::optional<std::uint16_t> readLittleEndian16(std::size_t offset) const;

    /**
     * @brief Check the header of the current entry (type, path, announced size), without
     *        reading its content
     * @param libarchiveEntry The current libarchive entry
     * @return The entry, its content still empty, or std::nullopt if it is rejected
     */
    [[nodiscard]] std::optional<ArchiveEntry> readHeader(struct archive_entry *libarchiveEntry);

    /**
     * @brief Read the content of the current entry, within the limits
     * @param path The entry path, for the logs
     * @return The content, or std::nullopt upon failure or if a limit is exceeded
     */
    [[nodiscard]] std::optional<std::string> readContent(const std::string &path);

    /**
     * @brief Log why the archive is rejected
     * @param reason The reason
     * @return Always std::nullopt, to be returned by the caller
     */
    [[nodiscard]] std::nullopt_t reject(const std::string &reason) const;

    /**
     * @brief Quote an entry path for a rejection reason, its control characters escaped (see
     *        act::text::StringUtil::ToPrintableText): the path comes from the archive
     * @param path The entry path
     * @return The quoted path
     */
    [[nodiscard]] static std::string QuotePath(const std::string &path);

  private:
    /** @brief Size of the buffer the entry contents are read with */
    static constexpr std::size_t READ_CHUNK_SIZE = 8192;

    /** @brief Signature starting a ZIP end of central directory record */
    static constexpr std::string_view ZIP_END_RECORD_SIGNATURE = "PK\x05\x06";

    /** @brief Size of a ZIP end of central directory record, without its comment */
    static constexpr std::size_t ZIP_END_RECORD_SIZE = 22;

    /** @brief Offset of the number of entries on this disk, in the ZIP end record */
    static constexpr std::size_t ZIP_END_RECORD_ENTRIES_ON_DISK_OFFSET = 8;

    /** @brief Offset of the total number of entries, in the ZIP end record */
    static constexpr std::size_t ZIP_END_RECORD_TOTAL_ENTRIES_OFFSET = 10;

    /** @brief Offset of the comment length, in the ZIP end record */
    static constexpr std::size_t ZIP_END_RECORD_COMMENT_LENGTH_OFFSET = 20;

  private:
    /** @brief The archive bytes */
    const std::string &m_data;

    /** @brief The reading options */
    const ArchiveReaderOptions m_options;

    /** @brief The logger */
    const act::foundation::AbsLogger &m_logger;

    /**
     * @brief The libarchive reader, nullptr until open(), for an empty ZIP archive, and once the
     *        entries are read
     */
    struct archive *m_archive{nullptr};

    /** @brief The state of the reader */
    State m_state{State::OPEN};

    /** @brief True if the data is an empty ZIP archive, which libarchive does not recognize */
    bool m_isEmptyZip{false};

    /** @brief Total size of the contents read so far */
    std::size_t m_totalSize{0};

    /** @brief Paths read so far, with `/` separators, to reject duplicates */
    std::set<std::string> m_paths;
};
} // namespace act::archive
