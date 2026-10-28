// SPDX-FileCopyrightText: 2025 Ghislain Mangé <ghislain.mange@allcircuits.com>
// SPDX-FileCopyrightText: 2026 Anthony Loiseau <anthony.loiseau@allcircuits.com>
//
// SPDX-License-Identifier: LicenseRef-ALLCircuits-ACT-1.1

#pragma once

#include "act_files/types/file_name_charset.hpp"

#include <cstddef>
#include <string>

namespace act::foundation
{
class AbsLogger;
} // namespace act::foundation

/** @brief Set of file path helpers */
namespace act::files::FilePathUtil
{
/** @brief Options of @ref SanitizeFileName */
struct SanitizeFileNameOptions
{
    /** @brief Usual maximum length of a file name on Linux and Windows file systems */
    static constexpr std::size_t DEFAULT_MAX_LENGTH = 255;

    /**
     * @brief Character replacing each run of forbidden bytes
     * @note It must be allowed by the character set, and be neither `.` nor a space; otherwise
     *       `_` is used instead
     */
    char replacementChar{'_'};

    /**
     * @brief Maximum length of the result, in bytes
     * @note 0 is handled as 1
     */
    std::size_t maxLength{DEFAULT_MAX_LENGTH};

    /**
     * @brief Name returned when nothing remains of the given name, truncated to the maximum
     *        length
     * @pre It is a valid file name, once truncated: it is not sanitized
     */
    std::string fallbackName{"unnamed"};
};

/**
 * @brief Compare two file paths for equality
 * @param path1 The first file path
 * @param path2 The second file path
 * @param logger The logger to use for logging
 * @return True if the paths are equal, false otherwise
 */
[[nodiscard]] bool ArePathsEqual(const std::string &path1,
                                 const std::string &path2,
                                 const act::foundation::AbsLogger &logger);

/**
 * @brief Extract the filename from a file path
 * @param path The file path (can be relative or absolute)
 * @return The filename (including extension) without directory components
 * @note Returns the full path if it has no directory components
 * @note Returns empty string for empty input or paths ending with '/'
 */
[[nodiscard]] std::string GetFilename(const std::string &path);

/**
 * @brief Test if a file name is a device name Windows reserves
 * @details The reserved names are `CON`, `PRN`, `AUX`, `NUL`, `CONIN$`, `CONOUT$`, `COM0` to
 *          `COM9`, `COM¹` to `COM³`, `LPT0` to `LPT9` and `LPT¹` to `LPT³`, whatever their case,
 *          their extension and the spaces ending their stem: `con`, `Nul.txt`, `com1.tar.gz` and
 *          `con .txt` are all reserved. On the side of caution, like Python's
 *          `ntpath.isreserved()`, since the exact rules vary across Windows versions.
 * @param fileName The file name, without directory
 * @return True if the file name is reserved, false otherwise
 * @note `.` and `..` are not device names: they are not file names at all, and are not reported.
 */
[[nodiscard]] bool IsWindowsReservedName(const std::string &fileName);

/**
 * @brief Test if a file or directory name is one an operating system gives its own metadata
 * @details Such entries are never user content. The names are:
 *          - macOS, compared exactly, as written by macOS: `__MACOSX` (added by the Finder when
 *            compressing), the AppleDouble files starting with `._` (extended attributes, next to
 *            each file on non Apple file systems), `.DS_Store`, `.Spotlight-V100`, `.Trashes`,
 *            `.fseventsd`;
 *          - Windows, compared without case, as its file systems do: `Thumbs.db`, `ehthumbs.db`,
 *            `desktop.ini`, `$RECYCLE.BIN`, `System Volume Information`.
 * @param name The file or directory name, without separator
 * @return True if the name is operating system metadata, false otherwise
 * @note The hidden files in general (`.git`, `.env`...) are user content, and are not reported.
 */
[[nodiscard]] bool IsOperatingSystemMetadataName(const std::string &name);

/**
 * @brief Test if a path is operating system metadata, or lies under such an entry
 * @details The path is split on `/` and `\`, then each component is tested with
 *          @ref IsOperatingSystemMetadataName: `__MACOSX/folder/data.csv`, `folder/._data.csv` and
 *          `C:\Users\Thumbs.db` are all reported.
 * @param path The path, relative or absolute
 * @return True if a component of the path is operating system metadata, false otherwise
 */
[[nodiscard]] bool IsOperatingSystemMetadataPath(const std::string &path);

/**
 * @brief Turn any string into a file name which is valid on Linux and Windows file systems
 * @details The name is processed byte by byte:
 *          1. each run of bytes the character set forbids is replaced by a single
 *             replacement character;
 *          2. the spaces are removed at both ends, then the dots, spaces and replacement
 *             characters at the end (Windows forbids a name ending with a dot or a space, and
 *             `.` and `..` are not names); leading dots are kept, as in `.bashrc`;
 *          3. the name is truncated to the maximum length, and its end trimmed again;
 *          4. the fallback name is returned if nothing remains;
 *          5. a name Windows reserves (see @ref IsWindowsReservedName) gets the replacement
 *             character after its stem: `con.txt` gives `con_.txt`. When the name is already at
 *             its maximum length, the last character of its stem is replaced instead, and the
 *             fallback name is returned if the name is still reserved.
 * @param name The string to turn into a file name
 * @param charset The character set the file name is restricted to
 * @param logger The logger to use for logging the invalid options
 * @param options The sanitizing options
 * @return The file name
 * @note Distinct names may give the same file name: making the names unique is left to the
 *       caller. Compare them without case if they are meant for Windows.
 */
[[nodiscard]] std::string SanitizeFileName(const std::string &name,
                                           FileNameCharset::Enum charset,
                                           const act::foundation::AbsLogger &logger,
                                           const SanitizeFileNameOptions &options = {});
} // namespace act::files::FilePathUtil
