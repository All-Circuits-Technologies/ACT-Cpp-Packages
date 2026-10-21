// SPDX-FileCopyrightText: 2026 Anthony Loiseau <anthony.loiseau@allcircuits.com>
//
// SPDX-License-Identifier: LicenseRef-ALLCircuits-ACT-1.1

#pragma once

#include "act_archive/types/entry_type.hpp"

#include <string>

namespace act::archive
{
/** @brief Entry read from an archive */
struct ArchiveEntry
{
    /**
     * @brief Path of the entry inside the archive, in UTF-8, without trailing separator
     * @note It is safe (see ArchivePathUtil::IsSafeEntryPath).
     */
    std::string path;

    /** @brief Type of the entry */
    EntryType type{EntryType::REGULAR_FILE};

    /** @brief Content of the entry, empty for a directory */
    std::string content;
};
} // namespace act::archive
