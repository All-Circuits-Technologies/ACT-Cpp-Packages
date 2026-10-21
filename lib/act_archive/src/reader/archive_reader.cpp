// SPDX-FileCopyrightText: 2026 Anthony Loiseau <anthony.loiseau@allcircuits.com>
//
// SPDX-License-Identifier: LicenseRef-ALLCircuits-ACT-1.1

#include "act_archive/reader/archive_reader.hpp"

#include "reader/archive_reader_impl.hpp"

#include <utility>

namespace act::archive
{
ArchiveReader::ArchiveReader(std::unique_ptr<Impl> impl)
    : m_impl(std::move(impl))
{
}

ArchiveReader::~ArchiveReader() = default;

std::unique_ptr<ArchiveReader> ArchiveReader::OpenFromMemory(
    const std::string &data,
    const ArchiveReaderOptions &options,
    const act::foundation::AbsLogger &logger)
{
    auto impl = std::make_unique<Impl>(data, options, logger);
    if (!impl->open())
    {
        return nullptr;
    }

    return std::unique_ptr<ArchiveReader>(new ArchiveReader(std::move(impl)));
}

std::optional<std::vector<ArchiveEntry>> ArchiveReader::readAllEntries()
{
    return m_impl->readAllEntries();
}
} // namespace act::archive
