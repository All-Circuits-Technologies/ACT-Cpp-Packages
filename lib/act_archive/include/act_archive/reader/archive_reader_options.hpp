// SPDX-FileCopyrightText: 2026 Anthony Loiseau <anthony.loiseau@allcircuits.com>
//
// SPDX-License-Identifier: LicenseRef-ALLCircuits-ACT-1.1

#pragma once

#include "act_archive/reader/archive_reader_limits.hpp"
#include "act_archive/types/archive_format.hpp"

#include <vector>

namespace act::archive
{
/** @brief Options of an ArchiveReader */
struct ArchiveReaderOptions
{
    /**
     * @brief Formats the archive may have, mandatory and not empty
     * @note Only these formats are recognized: the data of any other one is rejected.
     */
    std::vector<ArchiveFormat> allowedFormats;

    // Extension point: std::vector<ArchiveFilter> allowedFilters{ArchiveFilter::NONE}, see README

    /** @brief Limits of the archive */
    ArchiveReaderLimits limits{};
};
} // namespace act::archive
