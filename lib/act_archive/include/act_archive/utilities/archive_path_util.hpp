// SPDX-FileCopyrightText: 2026 Anthony Loiseau <anthony.loiseau@allcircuits.com>
//
// SPDX-License-Identifier: LicenseRef-ALLCircuits-ACT-1.1

#pragma once

#include <string>

/** @brief Set of helpers about the paths of archive entries */
namespace act::archive::ArchivePathUtil
{
/**
 * @brief Test if an entry path can be written to or extracted from an archive safely
 * @details A safe path stays inside the directory the archive is extracted to, on Linux and on
 *          Windows. Both `/` and `\` are taken as separators, as Windows does, and the path:
 *          - is not empty, and does not start with a separator;
 *          - holds no `:` (a Windows drive letter or alternate data stream) and no control
 *            character;
 *          - has no empty, `.` nor `..` component, so no trailing separator either.
 * @param path The entry path, in UTF-8
 * @return True if the path is safe, false otherwise
 * @note `a\b` is safe: a sub-directory on Windows, a plain name on Linux.
 * @note It does not check that the name is valid on Windows: see FilePathUtil::SanitizeFileName.
 */
[[nodiscard]] bool IsSafeEntryPath(const std::string &path);

/**
 * @brief Give an entry path with `/` as the only separator
 * @param path The entry path
 * @return The path, each `\` replaced by `/`
 * @note Two entries whose normalized paths are equal would be the same file on Windows.
 * @note Meant for archive entry paths only, not for file system paths: on Linux, `\` is a valid
 *       character of a file name.
 */
[[nodiscard]] std::string NormalizeEntrySeparators(const std::string &path);

/** @brief Separator of the entry paths in an archive */
constexpr char SEPARATOR = '/';

/** @brief Windows separator, taken as a separator too when checking an entry path */
constexpr char WINDOWS_SEPARATOR = '\\';
} // namespace act::archive::ArchivePathUtil
