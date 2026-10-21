// SPDX-FileCopyrightText: 2026 Anthony Loiseau <anthony.loiseau@allcircuits.com>
//
// SPDX-License-Identifier: LicenseRef-ALLCircuits-ACT-1.1

#pragma once

#include "act_archive/writer/abs_archive_sink.hpp"

#include <string>

namespace act::archive
{
/** @brief Archive sink keeping the archive in memory */
class StringArchiveSink : public AbsArchiveSink
{
  public:
    /** @copydoc AbsArchiveSink::write */
    [[nodiscard]] bool write(const std::string &chunk) override;

    /**
     * @brief Give the bytes written so far, and empty the sink
     * @return The archive bytes
     */
    [[nodiscard]] std::string takeData();

  private:
    /** @brief The bytes written so far */
    std::string m_data;
};
} // namespace act::archive
