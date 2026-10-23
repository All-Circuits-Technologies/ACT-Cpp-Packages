// SPDX-FileCopyrightText: 2026 Anthony Loiseau <anthony.loiseau@allcircuits.com>
//
// SPDX-License-Identifier: LicenseRef-ALLCircuits-ACT-1.1

#pragma once

#include "act_archive/reader/archive_reader_options.hpp"
#include "act_archive/types/archive_entry.hpp"
#include "act_foundation/not_copiable_not_movable.hpp"

#include <memory>
#include <optional>
#include <string>
#include <vector>

namespace act::foundation
{
class AbsLogger;
} // namespace act::foundation

namespace act::archive
{
/**
 * @brief Read an archive, which may come from an untrusted source
 * @details A reader is opened on an archive, then reads its entries. Only the allowed formats
 *          are recognized, and the entries are checked (types, paths, limits) before being
 *          given.
 * @note An archive held in an entry is read by opening another reader on the entry content.
 */
class ArchiveReader : private act::foundation::NotCopiableNotMovable
{
  private:
    /** @brief Opaque implementation */
    class Impl;

  private:
    /**
     * @brief Wrap an opened implementation, see OpenFromMemory()
     * @param impl The implementation
     */
    explicit ArchiveReader(std::unique_ptr<Impl> impl);

  public:
    /** @brief Release the archive */
    ~ArchiveReader() override;

  public:
    /**
     * @brief Open an archive held in memory
     * @param data The archive bytes; they are not copied and must outlive the reader
     * @param options The reading options, copied
     * @param logger The logger to use; it must outlive the reader
     * @return The reader, or nullptr if no format is allowed, or if the data is not in an
     *         allowed format or is damaged (the reason is logged)
     */
    [[nodiscard]] static std::unique_ptr<ArchiveReader> OpenFromMemory(
        const std::string &data,
        const ArchiveReaderOptions &options,
        const act::foundation::AbsLogger &logger);

    /**
     * @brief Forbid opening a temporary: the reader keeps a reference to the data, which would
     *        dangle
     */
    static std::unique_ptr<ArchiveReader> OpenFromMemory(std::string &&data,
                                                         const ArchiveReaderOptions &options,
                                                         const act::foundation::AbsLogger &logger) =
        delete;

  public:
    /**
     * @brief Read all the entries, their contents included
     * @details The reading is all or nothing: the archive is rejected as a whole when it:
     *          - holds an entry which is not a regular file or a directory (link, device...);
     *          - holds an encrypted entry;
     *          - holds an entry whose path is not safe (see ArchivePathUtil::IsSafeEntryPath) or
     *            cannot be decoded, or two entries with the same path (`/` and `\` being the
     *            same separator);
     *          - exceeds one of the limits.
     * @return The entries, in archive order, or std::nullopt if the archive is rejected (the
     *         reason is logged)
     * @note The entries can be read once: any further call fails.
     */
    [[nodiscard]] std::optional<std::vector<ArchiveEntry>> readAllEntries();

    // Extension point: nextEntry() / readContent() to read entry by entry

  private:
    /** @brief The implementation, keeping the archive library out of this header */
    std::unique_ptr<Impl> m_impl;
};
} // namespace act::archive
