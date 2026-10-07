<!-- SPDX-FileCopyrightText: 2026 Théo Magne <theo.magne@allcircuits.com> -->

<!-- SPDX-License-Identifier: LicenseRef-ALLCircuits-ACT-1.1 -->

# ACT C++ Libraries <!-- omit from toc -->

A collection of modular, reusable C++ libraries for embedded Linux systems and general-purpose
applications developed by All-Circuits Technologies.

## Table of content <!-- omit from toc -->

- [Overview](#overview)
- [Libraries](#libraries)
- [Requirements](#requirements)
  - [Dependencies](#dependencies)
- [Building](#building)
  - [Build All Libraries](#build-all-libraries)
  - [Build Options](#build-options)
  - [Run the Tests](#run-the-tests)
  - [Use Individual Libraries](#use-individual-libraries)
- [Development](#development)
  - [Development Container](#development-container)
  - [Code Style](#code-style)
- [License](#license)
- [Contributing](#contributing)
- [Maintainers](#maintainers)

## Overview

This repository is a collection of modular, reusable C++ libraries for embedded Linux systems and
general-purpose applications developed by All-Circuits Technologies.

## Libraries

| Library                                          | Description                                                                                  |
| ------------------------------------------------ | -------------------------------------------------------------------------------------------- |
| [`act_db_core`](lib/act_db_core/README.md)       | Database engine-agnostic core with connection management, query execution, and transactions. |
| [`act_db_sqlite`](lib/act_db_sqlite/README.md)   | SQLite database management with abstraction layer.                                           |
| [`act_files`](lib/act_files/README.md)           | File system operations and file management utilities.                                        |
| [`act_foundation`](lib/act_foundation/README.md) | Foundational base classes, constants, and design patterns (no dependencies).                 |
| [`act_linux_io`](lib/act_linux_io/README.md)     | Linux-specific I/O for GPIO control and LED management.                                      |
| [`act_logger`](lib/act_logger/README.md)         | Logging infrastructure with flexible output backends.                                        |
| [`act_system`](lib/act_system/README.md)         | System-level utilities for multi-process synchronization, reboots, and command execution.    |
| [`act_text`](lib/act_text/README.md)             | Text and string processing utilities.                                                        |
| [`act_threading`](lib/act_threading/README.md)   | Threading utilities and synchronization primitives.                                          |
| [`act_time`](lib/act_time/README.md)             | Date, time, and timing utilities with timers and delays.                                     |

## Requirements

- **C++ Standard:** C++20 or later
- **CMake:** Version 3.23 or later (3.25 or later to use the presets)
- **Compiler:** GCC 9+ or Clang 10+
- **OS:** Linux (some libraries are Linux-specific)

### Dependencies

Each library has its own dependencies. Refer to individual library READMEs for specific requirements.

Common external dependencies:

- libgpiod v2.x (for `act_linux_io`)
- SQLiteCpp (for `act_db_sqlite`)
- GoogleTest (for the unit tests only, see [Run the Tests](#run-the-tests))

## Building

### Build All Libraries

```bash
mkdir build
cd build
cmake ..
make
```

### Build Options

- `BUILD_DEMOS`: Build demo applications (default: OFF)
- `BUILD_TESTS`: Build the unit tests (default: OFF)

Example:

```bash
cmake -DBUILD_DEMOS=ON -DBUILD_TESTS=ON ..
make
```

### Run the Tests

The unit tests use [GoogleTest](https://github.com/google/googletest) and are registered with
CTest. Each library has its own test executable, built from `tests/<module>_tests/test_*.cpp`.

The `dev` preset of [`CMakePresets.json`](CMakePresets.json) builds the libraries and the tests in
`build/dev/`. It fetches a pinned GoogleTest release through the dependency provider of
[`cmake/dependencies/fetch_provider.cmake`](cmake/dependencies/fetch_provider.cmake), so only the
dependencies of the libraries themselves need to be installed (see `tools/install_deps.sh`):

```bash
cmake --preset dev
cmake --build --preset dev
ctest --preset dev
```

A build configured without the presets (`cmake -DBUILD_TESTS=ON ..`) looks for GoogleTest on the
system instead.

The CI runs the same three commands on every push.

### Use Individual Libraries

Each library can be integrated into your CMake project using `add_subdirectory()`:

```cmake
add_subdirectory(path/to/act_cpp_packages/lib/act_logger)
target_link_libraries(your_target PRIVATE act_logger)
```

## Development

### Development Container

A development container configuration is provided in [`.devcontainer/`](.devcontainer/) for VSCode
with all required tools and dependencies pre-installed.

### Code Style

- Code formatting: `.clang-format`
- Static analysis: `.clang-tidy`
- Markdown linting: `.markdownlint.yaml`

## License

This project is licensed under the AllCircuits ACT License 1.1. See
[LICENSES/LicenseRef-ALLCircuits-ACT-1.1.txt](LICENSES/LicenseRef-ALLCircuits-ACT-1.1.txt) for
details.

## Contributing

This is an internal All-Circuits Technologies repository. For contribution guidelines, please
contact the development team.

## Maintainers

- Théo Magne <theo.magne@allcircuits.com>
- All-Circuits Technologies team
