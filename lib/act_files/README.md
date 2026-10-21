<!-- SPDX-FileCopyrightText: 2026 Théo Magne <theo.magne@allcircuits.com> -->

<!-- SPDX-License-Identifier: LicenseRef-ALLCircuits-ACT-1.1 -->

# act_files

File system operations and file management utilities for ACT C++ modules.

## Description

This module provides utilities for file I/O operations, path manipulation, and file management.

## Contents

- **ExtFile:** Extended file abstraction with automatic cleanup
- **FileNameCharset:** Character sets a sanitized file name may be restricted to
- **FilePathUtil:** File path utilities (comparison, file name extraction, file name sanitizing)
- **FileUtil:** File system utilities (read, write, exists, etc.)

## Dependencies

- act_foundation
- act_logger
- act_text

## Usage

```cpp
#include "act_files/ext_file.hpp"
#include "act_files/file_path_util.hpp"
#include "act_files/file_util.hpp"
```

## License

LicenseRef-ALLCircuits-ACT-1.1
