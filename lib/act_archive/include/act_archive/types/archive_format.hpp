// SPDX-FileCopyrightText: 2026 Anthony Loiseau <anthony.loiseau@allcircuits.com>
//
// SPDX-License-Identifier: LicenseRef-ALLCircuits-ACT-1.1

#pragma once

namespace act::archive
{
/**
 * @brief Archive formats, which lay out the entries of an archive
 * @note Adding a format is described in the README of act_archive.
 */
enum class ArchiveFormat
{
    /** @brief ZIP archive, entries compressed with Deflate or stored */
    ZIP,
};
} // namespace act::archive
