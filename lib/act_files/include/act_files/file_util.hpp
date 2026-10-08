/**
 * SPDX-FileCopyrightText: 2025 Ghislain Mangé <ghislain.mange@allcircuits.com>
 *
 * SPDX-License-Identifier: LicenseRef-ALLCircuits-ACT-1.1
 */

#pragma once

#include <cstdio>
#include <iostream>
#include <memory>
#include <optional>
#include <string>

namespace act::foundation
{
class AbsLogger;
} // namespace act::foundation

namespace act::files
{
class ExtFile;
} // namespace act::files

/** @brief Set of File helpers */
namespace act::files::FileUtil
{
/**
 * @brief Read a file and return its content
 * @param path The file path
 * @param logger The logger to use for logging
 * @param mode The file open mode
 * @return The file content
 */
std::optional<std::string> ReadFile(const std::string &path,
                                    const act::foundation::AbsLogger &logger,
                                    std::ios::openmode mode = std::ios::in);

/**
 * @brief Read an integer from a file
 * @param path The path to the file
 * @param logger The logger to use for logging errors
 * @param mode The file open mode
 * @return The integer read from the file, or empty optional upon failure
 * @note The file must hold a decimal number, in ASCII or UTF-8 (a leading UTF-8 byte order mark
 *       is skipped), possibly surrounded by whitespace. Anything else fails, an UTF-16 file
 *       included.
 */
std::optional<int> ReadFileAsInt(const std::string &path,
                                 const act::foundation::AbsLogger &logger,
                                 std::ios::openmode mode = std::ios::in);

/** @brief Write content to a file
 * @param path The file path
 * @param content The content to write
 * @param logger The logger to use for logging
 * @param mode The file open mode
 * @return True if successful, false otherwise
 */
bool WriteFile(const std::string &path,
               const std::string &content,
               const act::foundation::AbsLogger &logger,
               std::ios::openmode mode = std::ios::out);

/**
 * @brief Write a stringifiable value to a file
 * @param path The path to the file
 * @param value The value to write as string
 * @param logger The logger to use for logging errors
 * @param mode The file open mode
 * @return True on success, false on failure
 */
template <typename T>
bool WriteFile(const std::string &path,
               const T &value,
               const act::foundation::AbsLogger &logger,
               std::ios::openmode mode = std::ios::out)
{
    return WriteFile(path, std::to_string(value), logger, mode);
}

/**
 * @brief Create and open a file
 * @param path The file path
 * @param logger The logger to use for logging
 * @param mode The file open mode
 * @param isTemp Whether the file is temporary
 * @return The file handle, or nullptr upon failure
 */
std::shared_ptr<ExtFile> CreateFile(const std::string &path,
                                    const act::foundation::AbsLogger &logger,
                                    std::ios::openmode mode,
                                    bool isTemp = false);

/**
 * @brief Create a file, and doesn't open it
 * @param path The file path
 * @param logger The logger to use for logging
 * @param isTemp Whether the file is temporary
 * @return The file handle, or nullptr upon failure
 */
std::shared_ptr<ExtFile> CreateFile(const std::string &path,
                                    const act::foundation::AbsLogger &logger,
                                    bool isTemp = false);

/**
 * @brief Compare two file paths for equality
 * @param path1 The first file path
 * @param path2 The second file path
 * @param logger The logger to use for logging
 * @return True if the paths are equal, false otherwise
 */
bool ArePathsEqual(const std::string &path1,
                   const std::string &path2,
                   const act::foundation::AbsLogger &logger);

/**
 * @brief Extract the filename from a file path
 * @param path The file path (can be relative or absolute)
 * @return The filename (including extension) without directory components
 * @note Returns the full path if it has no directory components
 * @note Returns empty string for empty input or paths ending with '/'
 */
std::string GetFilename(const std::string &path);
} /* namespace act::files::FileUtil */
