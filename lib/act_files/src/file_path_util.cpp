// SPDX-FileCopyrightText: 2025 Ghislain Mangé <ghislain.mange@allcircuits.com>
// SPDX-FileCopyrightText: 2026 Anthony Loiseau <anthony.loiseau@allcircuits.com>
//
// SPDX-License-Identifier: LicenseRef-ALLCircuits-ACT-1.1

#include "act_files/file_path_util.hpp"

#include "act_foundation/logger/abs_logger.hpp"

#include <filesystem>

namespace act::files::FilePathUtil
{
bool ArePathsEqual(const std::string &path1,
                   const std::string &path2,
                   const act::foundation::AbsLogger &logger)
{
    if (path1 == path2)
    {
        // No need to check further if the strings are identical
        return true;
    }

    bool isEqual = false;
    try
    {
        // A relative path is resolved against the current directory
        isEqual = std::filesystem::equivalent(path1, path2);
    }
    catch (const std::exception &e)
    {
        logger.warningStream() << "ArePathsEqual: Exception occurred while comparing paths '"
                               << path1 << "' and '" << path2 << "': " << e.what();
    }

    return isEqual;
}

std::string GetFilename(const std::string &path)
{
    if (path.empty())
    {
        return "";
    }

    std::filesystem::path fsPath(path);
    return fsPath.filename().string();
}
} // namespace act::files::FilePathUtil
