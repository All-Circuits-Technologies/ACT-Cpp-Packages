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

| Component          | Header                                        | Role                                            |
| ------------------ | --------------------------------------------- | ----------------------------------------------- |
| `ArchivePathUtil`  | `act_archive/utilities/archive_path_util.hpp` | Entry path safety check and separators          |
| `ArchiveEntry`     | `act_archive/types/archive_entry.hpp`         | Entry read from an archive: path, type, content |
| `ArchiveFormat`    | `act_archive/types/archive_format.hpp`        | Archive formats: `ZIP`                          |
| `EntryCompression` | `act_archive/types/entry_compression.hpp`     | Per entry compression hint: `DEFAULT`, `NONE`   |
| `EntryType`        | `act_archive/types/entry_type.hpp`            | Entry types: `REGULAR_FILE`, `DIRECTORY`        |

## CMake integration

```cmake
target_link_libraries(my_target PRIVATE act_archive)
```
