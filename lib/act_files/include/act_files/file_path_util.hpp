// SPDX-FileCopyrightText: 2025 Ghislain Mangé <ghislain.mange@allcircuits.com>
// SPDX-FileCopyrightText: 2026 Anthony Loiseau <anthony.loiseau@allcircuits.com>
//
// SPDX-License-Identifier: LicenseRef-ALLCircuits-ACT-1.1

#pragma once

#include <string>

namespace act::foundation
{
class AbsLogger;
} // namespace act::foundation

/** @brief Set of file path helpers */
namespace act::files::FilePathUtil
{
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
} // namespace act::files::FilePathUtil
