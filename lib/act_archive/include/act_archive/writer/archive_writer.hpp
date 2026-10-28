// SPDX-FileCopyrightText: 2026 Anthony Loiseau <anthony.loiseau@allcircuits.com>
//
// SPDX-License-Identifier: LicenseRef-ALLCircuits-ACT-1.1

#pragma once

#include "act_archive/writer/archive_writer_options.hpp"
#include "act_archive/writer/entry_options.hpp"
#include "act_foundation/not_copiable_not_movable.hpp"

#include <memory>
#include <string>

namespace act::foundation
{
class AbsLogger;
} // namespace act::foundation

namespace act::archive
{
class AbsArchiveSink;

/**
 * @brief Write an archive, entry after entry, to a sink
 * @details The entries are written in the order they are added; a directory needs not be added
 *          before the files it holds, but an archive is easier to browse when it is. The archive
 *          is complete once finalize() succeeds.
 *
 *          The entry paths take `/` as separator: a `\` is written as `/`. Files get the 0644
 *          permissions, directories the 0755 ones, and both the current time as modification
 *          time.
 * @note An archive can hold another one: write the inner archive to a StringArchiveSink, then add
 *       its bytes as a file of the outer archive, with EntryCompression::NONE.
 * @note An entry rejected for its path (unsafe, or already in the archive) leaves the archive
 *       untouched. Once writing failed (internal or sink error), or once the archive is
 *       finalized, any further operation fails.
 */
class ArchiveWriter : private act::foundation::NotCopiableNotMovable
{
  private:
    /** @brief Opaque implementation */
    class Impl;

  private:
    /**
     * @brief Wrap an opened implementation, see Create()
     * @param impl The implementation
     */
    explicit ArchiveWriter(std::unique_ptr<Impl> impl);

  public:
    /** @brief Abandon the archive if it is not finalized: nothing more is written to the sink */
    ~ArchiveWriter() override;

  public:
    /**
     * @brief Create a writer
     * @param options The archive options
     * @param sink The sink to write the archive to; it must outlive the writer
     * @param logger The logger to use; it must outlive the writer
     * @return The writer, or nullptr upon failure
     */
    [[nodiscard]] static std::unique_ptr<ArchiveWriter> Create(
        const ArchiveWriterOptions &options,
        AbsArchiveSink &sink,
        const act::foundation::AbsLogger &logger);

  public:
    /**
     * @brief Add a directory entry, without content
     * @param path The directory path, safe (see ArchivePathUtil::IsSafeEntryPath) once an
     *             optional trailing separator removed, in UTF-8
     * @return True on success, false otherwise
     */
    [[nodiscard]] bool addDirectoryEntry(const std::string &path);

    /**
     * @brief Add a regular file entry, given its content
     * @param path The file path, safe (see ArchivePathUtil::IsSafeEntryPath), in UTF-8
     * @param content The file content
     * @param options The entry options
     * @return True on success, false otherwise
     */
    [[nodiscard]] bool addFileFromMemory(const std::string &path,
                                         const std::string &content,
                                         const EntryOptions &options = {});

    /**
     * @brief Complete the archive and flush it to the sink
     * @return True on success, false otherwise
     */
    [[nodiscard]] bool finalize();

  private:
    /** @brief The implementation, keeping the archive library out of this header */
    std::unique_ptr<Impl> m_impl;
};
} // namespace act::archive
