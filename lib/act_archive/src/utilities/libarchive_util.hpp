// SPDX-FileCopyrightText: 2026 Anthony Loiseau <anthony.loiseau@allcircuits.com>
//
// SPDX-License-Identifier: LicenseRef-ALLCircuits-ACT-1.1

#pragma once

#include "act_archive/types/archive_format.hpp"
#include "act_archive/types/entry_compression.hpp"

#include <string>

struct archive;

/**
 * @brief Set of helpers around libarchive: error messages, and the mapping of the archive
 *        formats
 * @note Adding a format means handling it here: every switch has no default case, so the
 *       compiler lists them.
 * @note Internal to act_archive.
 */
namespace act::archive::LibarchiveUtil
{
/**
 * @brief Give the last error of a libarchive reader or writer
 * @param archive The libarchive reader or writer, nullptr accepted
 * @return The error message, without trailing line feed, or "unknown error"
 */
[[nodiscard]] std::string ErrorMessage(struct archive *archive);

/**
 * @brief Set the format of a libarchive writer
 * @param writer The libarchive writer, not opened yet
 * @param format The archive format
 * @return True on success, false otherwise (see archive_error_string())
 */
[[nodiscard]] bool SetWriterFormat(struct archive *writer, ArchiveFormat format);

/**
 * @brief Select the compression of the next entry of a libarchive writer
 * @param writer The libarchive writer
 * @param format The archive format, the one of the writer
 * @param compression The requested compression
 * @return True on success, false otherwise (see archive_error_string())
 */
[[nodiscard]] bool SetEntryCompression(struct archive *writer,
                                       ArchiveFormat format,
                                       EntryCompression compression);

/**
 * @brief Make a libarchive reader recognize a format
 * @param reader The libarchive reader, not opened yet
 * @param format The archive format to recognize
 */
void EnableReaderFormat(struct archive *reader, ArchiveFormat format);
} // namespace act::archive::LibarchiveUtil
