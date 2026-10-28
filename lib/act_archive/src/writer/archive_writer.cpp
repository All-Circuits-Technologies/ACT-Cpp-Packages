// SPDX-FileCopyrightText: 2026 Anthony Loiseau <anthony.loiseau@allcircuits.com>
//
// SPDX-License-Identifier: LicenseRef-ALLCircuits-ACT-1.1

#include "act_archive/writer/archive_writer.hpp"

#include "act_archive/writer/archive_writer_options.hpp"
#include "act_archive/writer/entry_options.hpp"
#include "writer/archive_writer_impl.hpp"

#include <memory>
#include <string>
#include <utility>

namespace act::archive
{
ArchiveWriter::ArchiveWriter(std::unique_ptr<Impl> impl)
    : m_impl(std::move(impl))
{
}

ArchiveWriter::~ArchiveWriter() = default;

std::unique_ptr<ArchiveWriter> ArchiveWriter::Create(const ArchiveWriterOptions &options,
                                                     AbsArchiveSink &sink,
                                                     const act::foundation::AbsLogger &logger)
{
    auto impl = std::make_unique<Impl>(sink, logger);
    if (!impl->open(options))
    {
        return nullptr;
    }

    // The constructor is private: std::make_unique cannot call it
    return std::unique_ptr<ArchiveWriter>(new ArchiveWriter(std::move(impl)));
}

bool ArchiveWriter::addDirectoryEntry(const std::string &path)
{
    return m_impl->addDirectoryEntry(path);
}

bool ArchiveWriter::addFileFromMemory(const std::string &path,
                                      const std::string &content,
                                      const EntryOptions &options)
{
    return m_impl->addFileFromMemory(path, content, options);
}

bool ArchiveWriter::finalize()
{
    return m_impl->finalize();
}
} // namespace act::archive
