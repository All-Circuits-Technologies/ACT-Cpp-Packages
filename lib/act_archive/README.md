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
compression filters (gzip, zstd...).

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
| `ArchivePathUtil`      | `act_archive/utilities/archive_path_util.hpp`   | Entry path safety check and separators             |
| `ArchiveEntry`         | `act_archive/types/archive_entry.hpp`           | Entry read from an archive: path, type, content    |
| `ArchiveFormat`        | `act_archive/types/archive_format.hpp`          | Archive formats: `ZIP`                             |
| `EntryCompression`     | `act_archive/types/entry_compression.hpp`       | Per entry compression hint: `DEFAULT`, `NONE`      |
| `EntryType`            | `act_archive/types/entry_type.hpp`              | Entry types: `REGULAR_FILE`, `DIRECTORY`           |

---

## Usage

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

---

## Security

`ArchiveReader` is meant for untrusted archives. The reading is all or nothing: one suspicious
entry rejects the whole archive.

- **Formats:** only the formats the caller allows at runtime, in
  `ArchiveReaderOptions::allowedFormats`, are recognized: a tar file is not read when only ZIP is
  allowed.
- **Paths:** each entry path must be safe, see `ArchivePathUtil::IsSafeEntryPath`.
- **Entry types:** only regular files and directories; links, devices and the other special files
  are rejected, as are the encrypted entries and the duplicated paths.
- **Sizes:** the entry count, the size of each entry and the total size are limited
  (`ArchiveReaderLimits`). The sizes are checked while decompressing, without trusting the archive.
- **Names:** non ASCII names are read and written in UTF-8, even when the process runs under the
  `C` locale; on Linux, this needs the `C.UTF-8` locale, built in glibc 2.35 or later.

## ZIP specifics

- Directories are stored with a trailing `/`; `ArchiveEntry::path` never has one.
- An empty archive is the 22 bytes of an end of central directory record, which `ArchiveReader`
  detects specially.

## CMake integration

```cmake
target_link_libraries(my_target PRIVATE act_archive)
```
