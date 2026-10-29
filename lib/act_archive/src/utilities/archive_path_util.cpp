// SPDX-FileCopyrightText: 2026 Anthony Loiseau <anthony.loiseau@allcircuits.com>
//
// SPDX-License-Identifier: LicenseRef-ALLCircuits-ACT-1.1

#include "act_archive/utilities/archive_path_util.hpp"

#include "act_text/vector_string_util.hpp"

#include <algorithm>
#include <string>
#include <string_view>
#include <vector>

namespace
{

/** @brief Windows drive letter (`C:`) and alternate data stream (`file:stream`) separator */
constexpr char WINDOWS_DRIVE_OR_STREAM_SEPARATOR = ':';

/** @brief Path component designating the current directory */
constexpr std::string_view CURRENT_DIRECTORY = ".";

/** @brief Path component designating the parent directory */
constexpr std::string_view PARENT_DIRECTORY = "..";

/** @brief Highest ASCII control character before the printable ones */
constexpr unsigned char LAST_CONTROL_CHAR = 0x1F;

/** @brief ASCII delete character, a control character too */
constexpr unsigned char DELETE_CHAR = 0x7F;

} // namespace

namespace act::archive::ArchivePathUtil
{
bool IsSafeEntryPath(const std::string &path)
{
    const bool hasForbiddenChar = std::ranges::any_of(path, [](char byte) {
        const auto unsignedByte = static_cast<unsigned char>(byte);
        return byte == WINDOWS_DRIVE_OR_STREAM_SEPARATOR || unsignedByte <= LAST_CONTROL_CHAR ||
               unsignedByte == DELETE_CHAR;
    });
    if (path.empty() || hasForbiddenChar)
    {
        return false;
    }

    // A leading, trailing or doubled separator gives an empty component
    const std::vector<std::string> components =
        act::text::VectorStringUtil::split(NormalizeEntrySeparators(path),
                                           std::string(1, SEPARATOR));

    return std::ranges::none_of(components, [](const std::string &component) {
        return component.empty() || component == CURRENT_DIRECTORY || component == PARENT_DIRECTORY;
    });
}

std::string NormalizeEntrySeparators(const std::string &path)
{
    std::string normalizedPath = path;
    std::ranges::replace(normalizedPath, WINDOWS_SEPARATOR, SEPARATOR);
    return normalizedPath;
}
} // namespace act::archive::ArchivePathUtil
