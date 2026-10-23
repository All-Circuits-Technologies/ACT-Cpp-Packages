<!-- SPDX-FileCopyrightText: 2026 Anthony Loiseau <anthony.loiseau@allcircuits.com> -->

<!-- SPDX-License-Identifier: LicenseRef-ALLCircuits-ACT-1.1 -->

# act_archive

Archive writing and reading for ACT C++ modules, on top of libarchive. It reads archives held in
memory and writes archives to a sink, such as a string in memory, nested archives included, with
safe entry paths and size limits.

## Dependencies

- act_foundation
- act_text
- libarchive (libarchive-dev), tested with 3.7.4

## Platforms

act_archive is only tested on Linux.
It is not tested on Windows: its implementation has provisions for Windows, but they are unchecked.
Supporting Windows needs a build there and the unit tests to pass, the archives with accented names
included.

## Supported formats

| Format | Writing | Reading |
| ------ | ------- | ------- |
| ZIP    | Yes     | Yes     |

No compression filter is supported yet. The API is ready for other formats (tar, 7z...) and
compression filters (gzip, zstd...), see
[Adding a format or a filter](#adding-a-format-or-a-filter).

### ZIP specifics

- Entry paths use `/` as separator: `ArchiveWriter` writes a `\` as `/`.
- Directories are stored with a trailing `/`; `ArchiveEntry::path` never has one.
- `EntryCompression::DEFAULT` is Deflate, `EntryCompression::NONE` stores the content.
- An empty archive is the 22 bytes of an end of central directory record, which `ArchiveReader`
  detects specially.

## Debian Packages

```bash
sudo apt install libarchive-dev
```

## Components

| Component              | Header                                          | Role                                               |
| ---------------------- | ----------------------------------------------- | -------------------------------------------------- |
| `ArchiveReader`        | `act_archive/reader/archive_reader.hpp`         | Opens an archive held in memory, reads its entries |
| `ArchiveReaderOptions` | `act_archive/reader/archive_reader_options.hpp` | Options of a reader: allowed formats, limits       |
| `ArchiveReaderLimits`  | `act_archive/reader/archive_reader_limits.hpp`  | Entry count and size limits of a reader            |
| `ArchiveWriter`        | `act_archive/writer/archive_writer.hpp`         | Writes an archive, entry after entry, to a sink    |
| `ArchiveWriterOptions` | `act_archive/writer/archive_writer_options.hpp` | Options of a writer: archive format                |
| `EntryOptions`         | `act_archive/writer/entry_options.hpp`          | Options of an entry: compression                   |
| `AbsArchiveSink`       | `act_archive/writer/abs_archive_sink.hpp`       | Destination of the archive bytes                   |
| `StringArchiveSink`    | `act_archive/writer/string_archive_sink.hpp`    | Sink keeping the archive in memory                 |
| `ArchivePathUtil`      | `act_archive/utilities/archive_path_util.hpp`   | Entry path safety check and separators             |
| `ArchiveEntry`         | `act_archive/types/archive_entry.hpp`           | Entry read from an archive: path, type, content    |
| `ArchiveFormat`        | `act_archive/types/archive_format.hpp`          | Archive formats: `ZIP`                             |
| `EntryCompression`     | `act_archive/types/entry_compression.hpp`       | Per entry compression hint: `DEFAULT`, `NONE`      |
| `EntryType`            | `act_archive/types/entry_type.hpp`              | Entry types: `REGULAR_FILE`, `DIRECTORY`           |

---

## Usage

### Write a ZIP archive

```cpp
#include "act_archive/writer/archive_writer.hpp"
#include "act_archive/writer/string_archive_sink.hpp"

using namespace act::archive;

StringArchiveSink sink;
std::unique_ptr<ArchiveWriter> writer =
    ArchiveWriter::Create({.format = ArchiveFormat::ZIP}, sink, logger);
if (!writer || !writer->addDirectoryEntry("My Folder") ||
    !writer->addFileFromMemory("My Folder/data.csv", csvContent) || !writer->finalize())
{
    return std::nullopt;
}

return sink.takeData();
```

A writer destroyed before `finalize()` abandons its archive: nothing more is written to the sink.

To nest an archive, add the bytes of the inner one as a file of the outer one, with
`EntryCompression::NONE` as it is already compressed.

### Read a ZIP archive received from a user

```cpp
#include "act_archive/reader/archive_reader.hpp"

using namespace act::archive;

ArchiveReaderOptions options{.allowedFormats = {ArchiveFormat::ZIP}};
options.limits.maxEntrySize = 1024U * 1024U;

std::unique_ptr<ArchiveReader> reader =
    ArchiveReader::OpenFromMemory(uploadedData, options, logger);
std::optional<std::vector<ArchiveEntry>> entries =
    (reader != nullptr) ? reader->readAllEntries() : std::nullopt;
if (!entries)
{
    return false; // the reason is logged
}
```

The data must outlive the reader. A nested archive is read by opening another reader on the
content of its entry.

When `readAllEntries()` rejects an archive, `getRejectionReason()` tells why, in English (for
instance `the entry 'a.bin' exceeds 1048576 bytes (announced size)`), to be reported as a technical
detail. When `OpenFromMemory()` fails, the reason is only logged: the data is not in an allowed
format, or is damaged.

---

## Security

`ArchiveReader` is meant for untrusted archives. The reading is all or nothing: one suspicious
entry rejects the whole archive.

- **Formats:** only the formats the caller allows at runtime, in
  `ArchiveReaderOptions::allowedFormats`, are recognized: a tar file is not read when only ZIP is
  allowed.
- **Paths:** each entry path must be safe, see `ArchivePathUtil::IsSafeEntryPath`. `/` and `\` are
  both separators: the paths are given with `/` only. The paths quoted in the logs have their
  control characters escaped.
- **Entry types:** only regular files and directories; links, devices and the other special files
  are rejected, as are the encrypted entries and the duplicated paths.
- **Sizes:** the entry count, the size of each entry and the total size are limited
  (`ArchiveReaderLimits`). The sizes are checked while decompressing, without trusting the archive.
- **Names:** non ASCII names are read and written in UTF-8, even when the process runs under the
  `C` locale: writing and reading switch the calling thread to the `C.UTF-8` locale meanwhile,
  under which libarchive converts the names to and from UTF-8, even when built without iconv.
  This locale must be installed on Linux (glibc provides it since 2.35, Debian's `libc-bin` ships
  it). Without it, the archives with non ASCII names are rejected, and such names can't be
  written.

`ArchiveWriter` checks the same paths, rejects duplicates and the paths which are not UTF-8, so
that it never writes an archive `ArchiveReader` would reject. It does not check that a name is
valid on Windows: see `FilePathUtil::SanitizeFileName` in act_files.

## Adding a format or a filter

The API separates the format, which lays the entries out (ZIP, tar, 7z), from the filter,
which compresses the whole stream (gzip, zstd, xz): a `.tar.zst` archive is the TAR format with
the ZSTD filter.

To add a format:

1. add its value to `ArchiveFormat`;
2. handle it in the `switch` statements of `src/utilities/libarchive_util.cpp` (writer
   format, per entry compression, formats the reader recognizes);
3. add tests, an archive of another tool included.

To add the first filter, replace the "Extension point" comments of `ArchiveWriterOptions` and
`ArchiveReaderOptions` by the fields they describe, defaulting to `NONE`, then map the values to
`archive_write_add_filter_*()` and `archive_read_support_filter_*()`, and reject the invalid
format and filter combinations when creating a writer. Existing callers are not affected.

## CMake integration

```cmake
target_link_libraries(my_target PRIVATE act_archive)
```
