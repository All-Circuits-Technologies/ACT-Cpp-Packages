// SPDX-FileCopyrightText: 2026 Anthony Loiseau <anthony.loiseau@allcircuits.com>
//
// SPDX-License-Identifier: LicenseRef-ALLCircuits-ACT-1.1

#include "utilities/libarchive_util.hpp"

#include "act_text/string_util.hpp"

#include <archive.h>

namespace
{

/** @brief Message given when libarchive has no error message */
constexpr const char *UNKNOWN_ERROR_MESSAGE = "unknown error";

/** @brief Name of the libarchive ZIP format, for its options */
constexpr const char *ZIP_FORMAT_NAME = "zip";

/** @brief libarchive option giving the character set of the entry names */
constexpr const char *HEADER_CHARSET_OPTION = "hdrcharset";

/** @brief The UTF-8 character set, for the hdrcharset option */
constexpr const char *UTF8_CHARSET = "UTF-8";

} // namespace

namespace act::archive::LibarchiveUtil
{
std::string ErrorMessage(struct archive *archive)
{
    const char *error = (archive != nullptr) ? archive_error_string(archive) : nullptr;
    if (error == nullptr)
    {
        return UNKNOWN_ERROR_MESSAGE;
    }

    // Some libarchive messages end with a line feed, unwelcome in a log line
    return act::text::StringUtil::RtrimCopy(error);
}

bool SetWriterFormat(struct archive *writer, ArchiveFormat format)
{
    int result = ARCHIVE_FATAL;

    switch (format)
    {
        case ArchiveFormat::ZIP:
            result = archive_write_set_format_zip(writer);
            if (result == ARCHIVE_OK)
            {
                // Entry names in UTF-8, flagged as such (general purpose bit 11), whatever the
                // locale: without it, libarchive only flags them under a UTF-8 locale. Reading
                // needs more, see ScopedUtf8Locale
                result = archive_write_set_format_option(writer,
                                                         ZIP_FORMAT_NAME,
                                                         HEADER_CHARSET_OPTION,
                                                         UTF8_CHARSET);
            }
            break;
    }

    return result == ARCHIVE_OK;
}

bool SetEntryCompression(struct archive *writer, ArchiveFormat format, EntryCompression compression)
{
    int result = ARCHIVE_FATAL;

    switch (format)
    {
        case ArchiveFormat::ZIP:
            switch (compression)
            {
                case EntryCompression::DEFAULT:
                    result = archive_write_zip_set_compression_deflate(writer);
                    break;

                case EntryCompression::NONE:
                    result = archive_write_zip_set_compression_store(writer);
                    break;
            }
            break;
    }

    return result == ARCHIVE_OK;
}

bool EnableReaderFormat(struct archive *reader, ArchiveFormat format)
{
    int result = ARCHIVE_FATAL;

    switch (format)
    {
        case ArchiveFormat::ZIP:
            // The seekable reader trusts the central directory, like the other ZIP tools, and
            // knows the entry sizes before decompressing them
            result = archive_read_support_format_zip_seekable(reader);
            break;
    }

    return result == ARCHIVE_OK;
}
} // namespace act::archive::LibarchiveUtil
