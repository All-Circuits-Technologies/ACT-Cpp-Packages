// SPDX-FileCopyrightText: 2026 Anthony Loiseau <anthony.loiseau@allcircuits.com>
//
// SPDX-License-Identifier: LicenseRef-ALLCircuits-ACT-1.1

#pragma once

#include "act_foundation/not_copiable_not_movable.hpp"

#include <string>

namespace act::archive
{
/**
 * @brief Destination of the bytes of an archive being written, see ArchiveWriter
 * @note StringArchiveSink keeps them in memory; a sink writing to a file or to a socket can be
 *       added without changing ArchiveWriter.
 */
class AbsArchiveSink : private act::foundation::NotCopiableNotMovable
{
  public:
    /**
     * @brief Append bytes of the archive
     * @param chunk The bytes to append, in order
     * @return True on success, false otherwise; the archive being written then fails
     */
    [[nodiscard]] virtual bool write(const std::string &chunk) = 0;
};
} // namespace act::archive
