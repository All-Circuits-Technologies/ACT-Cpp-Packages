// SPDX-FileCopyrightText: 2026 Anthony Loiseau <anthony.loiseau@allcircuits.com>
//
// SPDX-License-Identifier: LicenseRef-ALLCircuits-ACT-1.1

/**
 * @file raw_archive_builder.hpp
 * @brief Build archives with libarchive directly, bypassing the checks of ArchiveWriter.
 *
 * It crafts the archives ArchiveWriter refuses to write (unsafe paths, links, duplicates) or
 * cannot write (other formats), so that the tests can check ArchiveReader rejects them.
 */

#pragma once

#include <archive.h>
#include <archive_entry.h>

#include <memory>
#include <string>

namespace act::archive::tests
{
/** @brief Builder of raw archives, for the tests */
class RawArchiveBuilder
{
  public:
    /** @brief Formats the builder can write */
    enum class Format
    {
        ZIP,
        TAR,
    };

  public:
    /**
     * @brief Start an archive
     * @param format The archive format
     */
    explicit RawArchiveBuilder(Format format = Format::ZIP)
        : m_archive(archive_write_new(), &archive_write_free)
    {
        if (format == Format::ZIP)
        {
            archive_write_set_format_zip(m_archive.get());
        }
        else
        {
            archive_write_set_format_pax_restricted(m_archive.get());
        }

        archive_write_set_bytes_in_last_block(m_archive.get(), 1);
        archive_write_open2(m_archive.get(), &m_data, nullptr, &AppendToString, nullptr, nullptr);
    }

    /**
     * @brief Add a regular file, its path written as is
     * @param path The raw path
     * @param content The content
     * @return The builder
     */
    RawArchiveBuilder &addFile(const std::string &path, const std::string &content)
    {
        return addEntry(path, AE_IFREG, content, {});
    }

    /**
     * @brief Add a directory, its path written as is
     * @param path The raw path
     * @return The builder
     */
    RawArchiveBuilder &addDirectory(const std::string &path)
    {
        return addEntry(path, AE_IFDIR, {}, {});
    }

    /**
     * @brief Add a symbolic link
     * @param path The raw path
     * @param target The link target
     * @return The builder
     */
    RawArchiveBuilder &addSymlink(const std::string &path, const std::string &target)
    {
        return addEntry(path, AE_IFLNK, {}, target);
    }

    /**
     * @brief Complete the archive
     * @return The archive bytes
     */
    std::string build()
    {
        archive_write_close(m_archive.get());
        return m_data;
    }

  private:
    static la_ssize_t AppendToString(struct archive * /*archive*/,
                                     void *clientData,
                                     const void *buffer,
                                     size_t length)
    {
        static_cast<std::string *>(clientData)->append(static_cast<const char *>(buffer), length);
        return static_cast<la_ssize_t>(length);
    }

    RawArchiveBuilder &addEntry(const std::string &path,
                                unsigned int fileType,
                                const std::string &content,
                                const std::string &symlinkTarget)
    {
        std::unique_ptr<struct archive_entry, decltype(&archive_entry_free)> entry(
            archive_entry_new(),
            &archive_entry_free);
        archive_entry_set_pathname_utf8(entry.get(), path.c_str());
        archive_entry_set_filetype(entry.get(), fileType);
        archive_entry_set_perm(entry.get(), PERMISSIONS);
        if (fileType == AE_IFREG)
        {
            archive_entry_set_size(entry.get(), static_cast<la_int64_t>(content.size()));
        }
        if (fileType == AE_IFLNK)
        {
            archive_entry_set_symlink_utf8(entry.get(), symlinkTarget.c_str());
        }

        archive_write_header(m_archive.get(), entry.get());
        if (!content.empty())
        {
            archive_write_data(m_archive.get(), content.data(), content.size());
        }

        return *this;
    }

  private:
    // mode_t does not exist with MSVC: use the type of the libarchive permissions
    static constexpr auto PERMISSIONS = static_cast<decltype(AE_IFREG)>(0644);

    std::string m_data;
    std::unique_ptr<struct archive, decltype(&archive_write_free)> m_archive;
};
} // namespace act::archive::tests
