// SPDX-FileCopyrightText: 2026 Anthony Loiseau <anthony.loiseau@allcircuits.com>
//
// SPDX-License-Identifier: LicenseRef-ALLCircuits-ACT-1.1

#pragma once

#include "act_archive/writer/archive_writer.hpp"
#include "act_foundation/not_copiable_not_movable.hpp"

#include <archive.h>
#include <archive_entry.h>

#include <set>
#include <string>

namespace act::foundation
{
class AbsLogger;
} // namespace act::foundation

namespace act::archive
{
class AbsArchiveSink;

/**
 * @brief Implementation of ArchiveWriter, wrapping a libarchive writer, freed by the destructor
 *        (RAII)
 */
class ArchiveWriter::Impl : private act::foundation::NotCopiableNotMovable
{
  private:
    /** @brief States of the writer */
    enum class State
    {
        /** @brief Entries can be added */
        OPEN,

        /** @brief The archive is complete */
        FINALIZED,

        /** @brief Writing failed: the archive is unusable */
        FAILED,
    };

    /**
     * @brief Type of the libarchive permissions: mode_t on Linux, but mode_t does not exist with
     *        MSVC, and libarchive uses another type on Windows and from its version 4
     */
    using LibarchiveMode = decltype(AE_IFREG);

  public:
    /**
     * @brief Prepare the writer, see open()
     * @param sink The sink to write the archive to
     * @param logger The logger to use
     */
    Impl(AbsArchiveSink &sink, const act::foundation::AbsLogger &logger);

    /** @brief Free the libarchive writer, see closeLibarchive() */
    ~Impl() override;

  public:
    /**
     * @brief Create the libarchive writer and open it on the sink
     * @param options The archive options
     * @return True on success, false otherwise
     */
    [[nodiscard]] bool open(const ArchiveWriterOptions &options);

    /** @copydoc ArchiveWriter::addDirectoryEntry */
    [[nodiscard]] bool addDirectoryEntry(const std::string &path);

    /** @copydoc ArchiveWriter::addFileFromMemory */
    [[nodiscard]] bool addFileFromMemory(const std::string &path,
                                         const std::string &content,
                                         const EntryOptions &options);

    /** @copydoc ArchiveWriter::finalize */
    [[nodiscard]] bool finalize();

  private:
    /**
     * @brief Create the libarchive writer, in the format of the archive, without padding
     * @return True on success, false otherwise (the reason is logged)
     */
    [[nodiscard]] bool createLibarchiveWriter();

    /**
     * @brief Close and free the libarchive writer, if any
     * @details Unless the archive is finalized, the archive is abandoned: nothing more reaches the
     *          sink, its end in particular.
     * @note Called twice, it does nothing the second time.
     */
    void closeLibarchive();

    /**
     * @brief Check that an entry can still be added, at a safe and new path
     * @param path The entry path, with `/` separators and without trailing separator
     * @return True if the entry can be added, false otherwise
     */
    [[nodiscard]] bool checkNewEntry(const std::string &path);

    /**
     * @brief Select the compression of the next entry
     * @param compression The requested compression
     * @return True on success, false otherwise
     */
    [[nodiscard]] bool selectCompression(EntryCompression compression);

    /**
     * @brief Write an entry header, then its content
     * @param path The entry path, checked
     * @param fileType The libarchive file type (AE_IFREG or AE_IFDIR)
     * @param permissions The entry permissions
     * @param content The entry content, empty for a directory
     * @return True on success, false otherwise
     */
    [[nodiscard]] bool writeEntry(const std::string &path,
                                  unsigned int fileType,
                                  LibarchiveMode permissions,
                                  const std::string &content);

    /**
     * @brief Mark the writer as failed and log why
     * @param operation The failed operation
     * @return Always false, to be returned by the caller
     */
    bool fail(const std::string &operation);

    /**
     * @brief Quote an entry path for a log, its control characters escaped (see
     *        act::text::StringUtil::ToPrintableText): the path may come from an untrusted source
     * @param path The entry path
     * @return The quoted path
     */
    [[nodiscard]] static std::string QuotePath(const std::string &path);

  private:
    /**
     * @brief libarchive write callback, forwarding the archive bytes to the sink
     * @param archive The libarchive writer
     * @param clientData The Impl
     * @param buffer The bytes to write
     * @param length The number of bytes to write
     * @return The number of bytes written, or -1 upon failure
     */
    static la_ssize_t WriteToSink(struct archive *archive,
                                  void *clientData,
                                  const void *buffer,
                                  size_t length);

  private:
    /** @brief Permissions of a regular file entry */
    static constexpr auto FILE_PERMISSIONS = static_cast<LibarchiveMode>(0644);

    /** @brief Permissions of a directory entry */
    static constexpr auto DIRECTORY_PERMISSIONS = static_cast<LibarchiveMode>(0755);

  private:
    /** @brief The sink the archive is written to */
    AbsArchiveSink &m_sink;

    /** @brief The logger */
    const act::foundation::AbsLogger &m_logger;

    /** @brief The libarchive writer, nullptr until open(), and once finalized or failed */
    struct archive *m_archive{nullptr};

    /** @brief The state of the writer */
    State m_state{State::OPEN};

    /** @brief True once the archive is abandoned: its remaining bytes are not given to the sink */
    bool m_isDiscardingOutput{false};

    /** @brief The archive format */
    ArchiveFormat m_format{ArchiveFormat::ZIP};

    /** @brief Paths already written, to reject duplicates */
    std::set<std::string> m_paths;
};
} // namespace act::archive
