// SPDX-FileCopyrightText: 2026 Anthony Loiseau <anthony.loiseau@allcircuits.com>
//
// SPDX-License-Identifier: LicenseRef-ALLCircuits-ACT-1.1

#pragma once

#include "act_archive/types/entry_compression.hpp"

namespace act::archive
{
/** @brief Options of an entry added to an ArchiveWriter */
struct EntryOptions
{
    /** @brief Compression of the entry content */
    EntryCompression compression{EntryCompression::DEFAULT};
};
} // namespace act::archive
