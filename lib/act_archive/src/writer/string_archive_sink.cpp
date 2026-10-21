// SPDX-FileCopyrightText: 2026 Anthony Loiseau <anthony.loiseau@allcircuits.com>
//
// SPDX-License-Identifier: LicenseRef-ALLCircuits-ACT-1.1

#include "act_archive/writer/string_archive_sink.hpp"

#include <utility>

namespace act::archive
{
bool StringArchiveSink::write(const std::string &chunk)
{
    m_data += chunk;
    return true;
}

std::string StringArchiveSink::takeData()
{
    return std::exchange(m_data, std::string());
}
} // namespace act::archive
