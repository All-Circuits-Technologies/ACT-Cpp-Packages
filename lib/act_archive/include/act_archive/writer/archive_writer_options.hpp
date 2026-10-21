// SPDX-FileCopyrightText: 2026 Anthony Loiseau <anthony.loiseau@allcircuits.com>
//
// SPDX-License-Identifier: LicenseRef-ALLCircuits-ACT-1.1

#pragma once

#include "act_archive/types/archive_format.hpp"

namespace act::archive
{
/** @brief Options of an ArchiveWriter */
struct ArchiveWriterOptions
{
    /** @brief Format of the archive, mandatory */
    ArchiveFormat format;

    // Extension point: ArchiveFilter filter{ArchiveFilter::NONE} (gzip, zstd...), see README
};
} // namespace act::archive
