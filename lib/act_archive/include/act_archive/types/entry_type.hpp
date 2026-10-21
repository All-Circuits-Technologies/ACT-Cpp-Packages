// SPDX-FileCopyrightText: 2026 Anthony Loiseau <anthony.loiseau@allcircuits.com>
//
// SPDX-License-Identifier: LicenseRef-ALLCircuits-ACT-1.1

#pragma once

namespace act::archive
{
/**
 * @brief Types of the archive entries act_archive handles
 * @note Links, devices and the other special files are rejected when reading.
 */
enum class EntryType
{
    /** @brief Regular file, with a content */
    REGULAR_FILE,

    /** @brief Directory, without content */
    DIRECTORY,
};
} // namespace act::archive
