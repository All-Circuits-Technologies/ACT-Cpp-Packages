// SPDX-FileCopyrightText: 2026 Anthony Loiseau <anthony.loiseau@allcircuits.com>
//
// SPDX-License-Identifier: LicenseRef-ALLCircuits-ACT-1.1

#include "writer/archive_writer_impl.hpp"

#include "act_archive/utilities/archive_path_util.hpp"
#include "act_archive/writer/abs_archive_sink.hpp"
#include "act_foundation/logger/abs_logger.hpp"
#include "act_text/string_util.hpp"
#include "utilities/libarchive_util.hpp"

#include <archive.h>
#include <archive_entry.h>

#include <cerrno>
#include <ctime>
#include <set>
#include <utility>

namespace act::archive
{
ArchiveWriter::Impl::Impl(AbsArchiveSink &sink, const act::foundation::AbsLogger &logger)
    : m_sink(sink),
      m_logger(logger)
{
}

ArchiveWriter::Impl::~Impl()
{
    closeLibarchive();
}

bool ArchiveWriter::Impl::open(const ArchiveWriterOptions &options)
{
    m_format = options.format;
    if (!createLibarchiveWriter())
    {
        return false;
    }

    // Open the writer on the sink: nothing is written to it before the first entry
    if (archive_write_open2(m_archive, this, nullptr, &Impl::WriteToSink, nullptr, nullptr) !=
        ARCHIVE_OK)
    {
        return fail("open");
    }

    return true;
}

bool ArchiveWriter::Impl::addDirectoryEntry(const std::string &path)
{
    std::string normalizedPath = ArchivePathUtil::NormalizeEntrySeparators(path);
    if (normalizedPath.ends_with(ArchivePathUtil::SEPARATOR))
    {
        normalizedPath.pop_back();
    }

    if (!checkNewEntry(normalizedPath))
    {
        return false;
    }

    // The entry names are flagged as UTF-8 whatever the locale, see LibarchiveUtil::SetWriterFormat
    return writeEntry(normalizedPath, AE_IFDIR, DIRECTORY_PERMISSIONS, {});
}

bool ArchiveWriter::Impl::addFileFromMemory(const std::string &path,
                                            const std::string &content,
                                            const EntryOptions &options)
{
    const std::string normalizedPath = ArchivePathUtil::NormalizeEntrySeparators(path);
    if (!checkNewEntry(normalizedPath))
    {
        return false;
    }

    return selectCompression(options.compression) &&
           writeEntry(normalizedPath, AE_IFREG, FILE_PERMISSIONS, content);
}

bool ArchiveWriter::Impl::finalize()
{
    if (m_state != State::OPEN)
    {
        m_logger.errorStream() << "ArchiveWriter: cannot finalize an archive which is "
                               << (m_state == State::FINALIZED ? "already finalized" : "failed");
        return false;
    }

    if (archive_write_close(m_archive) != ARCHIVE_OK)
    {
        return fail("finalize");
    }

    m_state = State::FINALIZED;

    // Nothing more can be written: release the libarchive writer right away
    closeLibarchive();
    return true;
}

bool ArchiveWriter::Impl::createLibarchiveWriter()
{
    m_archive = archive_write_new();
    if (m_archive == nullptr)
    {
        m_state = State::FAILED;
        m_logger.errorStream() << "ArchiveWriter: cannot create the libarchive writer";
        return false;
    }

    if (!LibarchiveUtil::SetWriterFormat(m_archive, m_format))
    {
        return fail("set the format");
    }

    // Without it, libarchive pads the output to a multiple of 10240 bytes
    if (archive_write_set_bytes_in_last_block(m_archive, 1) != ARCHIVE_OK)
    {
        return fail("disable the padding");
    }

    return true;
}

void ArchiveWriter::Impl::closeLibarchive()
{
    if (m_archive == nullptr)
    {
        return;
    }

    // archive_write_free() closes the archive, writing its end. Unless the archive is finalized,
    // discard it: archive_write_fail() would prevent the closing, but leak the libarchive buffers
    if (m_state != State::FINALIZED)
    {
        m_isDiscardingOutput = true;
    }

    archive_write_free(m_archive);
    m_archive = nullptr;
}

bool ArchiveWriter::Impl::checkNewEntry(const std::string &path)
{
    if (m_state != State::OPEN)
    {
        m_logger.errorStream() << "ArchiveWriter: cannot add " << QuotePath(path)
                               << " to an archive which is "
                               << (m_state == State::FINALIZED ? "finalized" : "failed");
        return false;
    }

    if (!ArchivePathUtil::IsSafeEntryPath(path))
    {
        m_logger.errorStream() << "ArchiveWriter: the entry path " << QuotePath(path)
                               << " is not safe";
        return false;
    }

    if (!m_paths.insert(path).second)
    {
        m_logger.errorStream() << "ArchiveWriter: the entry path " << QuotePath(path)
                               << " is already in the archive";
        return false;
    }

    return true;
}

bool ArchiveWriter::Impl::selectCompression(EntryCompression compression)
{
    if (!LibarchiveUtil::SetEntryCompression(m_archive, m_format, compression))
    {
        return fail("select the compression");
    }

    return true;
}

bool ArchiveWriter::Impl::writeEntry(const std::string &path,
                                     unsigned int fileType,
                                     LibarchiveMode permissions,
                                     const std::string &content)
{
    std::unique_ptr<struct archive_entry, decltype(&archive_entry_free)> entry(archive_entry_new(),
                                                                               &archive_entry_free);
    if (entry == nullptr)
    {
        return fail("create the entry " + QuotePath(path));
    }

    archive_entry_set_pathname_utf8(entry.get(), path.c_str());
    archive_entry_set_filetype(entry.get(), fileType);
    archive_entry_set_perm(entry.get(), permissions);
    archive_entry_set_mtime(entry.get(), std::time(nullptr), 0);

    if (fileType == AE_IFREG)
    {
        archive_entry_set_size(entry.get(), static_cast<la_int64_t>(content.size()));
    }

    if (archive_write_header(m_archive, entry.get()) != ARCHIVE_OK)
    {
        return fail("write the header of " + QuotePath(path));
    }

    if (!content.empty() && archive_write_data(m_archive, content.data(), content.size()) !=
                                static_cast<la_ssize_t>(content.size()))
    {
        return fail("write the content of " + QuotePath(path));
    }

    return true;
}

bool ArchiveWriter::Impl::fail(const std::string &operation)
{
    m_state = State::FAILED;

    m_logger.errorStream() << "ArchiveWriter: cannot " << operation << ": "
                           << LibarchiveUtil::ErrorMessage(m_archive);

    // The archive is unusable: release the libarchive writer right away
    closeLibarchive();
    return false;
}

std::string ArchiveWriter::Impl::QuotePath(const std::string &path)
{
    return "'" + act::text::StringUtil::ToPrintableText(path) + "'";
}

la_ssize_t ArchiveWriter::Impl::WriteToSink(struct archive *archive,
                                            void *clientData,
                                            const void *buffer,
                                            size_t length)
{
    auto *impl = static_cast<Impl *>(clientData);

    if (impl->m_isDiscardingOutput)
    {
        // The archive is abandoned: its remaining bytes never reach the sink. Report them as
        // written, like /dev/null does, so that libarchive completes its closing and frees all its
        // buffers
        return static_cast<la_ssize_t>(length);
    }

    if (!impl->m_sink.write(std::string(static_cast<const char *>(buffer), length)))
    {
        archive_set_error(archive, EIO, "the archive sink failed");
        return -1;
    }

    return static_cast<la_ssize_t>(length);
}
} // namespace act::archive
