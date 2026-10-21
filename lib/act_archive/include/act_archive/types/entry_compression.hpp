// SPDX-FileCopyrightText: 2026 Anthony Loiseau <anthony.loiseau@allcircuits.com>
//
// SPDX-License-Identifier: LicenseRef-ALLCircuits-ACT-1.1

#pragma once

namespace act::archive
{
/**
 * @brief Compression of a single archive entry, as a hint the archive format interprets
 * @note A format without per entry compression ignores it. Compressing a whole archive (a
 *       `.tar.gz`) is the role of the filters, see the README.
 */
enum class EntryCompression
{
    /** @brief The usual compression of the format: Deflate for ZIP */
    DEFAULT,

    /** @brief No compression, for an already compressed content: stored for ZIP */
    NONE,
};
} // namespace act::archive
