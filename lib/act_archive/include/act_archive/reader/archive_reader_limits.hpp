// SPDX-FileCopyrightText: 2026 Anthony Loiseau <anthony.loiseau@allcircuits.com>
//
// SPDX-License-Identifier: LicenseRef-ALLCircuits-ACT-1.1

#pragma once

#include <cstddef>

namespace act::archive
{
/** @brief Limits protecting against oversized archives, such as zip bombs */
struct ArchiveReaderLimits
{
    /** @brief Default maximum number of entries */
    static constexpr std::size_t DEFAULT_MAX_ENTRY_COUNT = 1000;

    /** @brief Default maximum size of an entry content, in bytes */
    static constexpr std::size_t DEFAULT_MAX_ENTRY_SIZE = std::size_t{16} * 1024U * 1024U;

    /** @brief Default maximum size of all the entry contents together, in bytes */
    static constexpr std::size_t DEFAULT_MAX_TOTAL_SIZE = std::size_t{64} * 1024U * 1024U;

    /** @brief Maximum number of entries, directories included */
    std::size_t maxEntryCount{DEFAULT_MAX_ENTRY_COUNT};

    /**
     * @brief Maximum size of an entry content, in bytes
     * @note Checked while decompressing: the size an archive announces is not trusted.
     */
    std::size_t maxEntrySize{DEFAULT_MAX_ENTRY_SIZE};

    /** @brief Maximum size of all the entry contents together, in bytes */
    std::size_t maxTotalSize{DEFAULT_MAX_TOTAL_SIZE};
};
} // namespace act::archive
