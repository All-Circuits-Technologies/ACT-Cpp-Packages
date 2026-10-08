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
  - [Use Individual Libraries](#use-individual-libraries)
- [Development](#development)
  - [Development Container](#development-container)
  - [Continuous Integration](#continuous-integration)
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
- **CMake:** Version 3.23 or later
- **Compiler:** GCC 9+ or Clang 10+
- **OS:** Linux (some libraries are Linux-specific)

### Dependencies

Each library has its own dependencies. Refer to individual library READMEs for specific requirements.

Common external dependencies:

- libgpiod v2.x (for `act_linux_io`)
- SQLiteCpp (for `act_db_sqlite`)

The Debian packages that provide them are listed in the toolchain image's
[`Dockerfile`](.devcontainer/image/Dockerfile), under "Package dependencies".

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
- `BUILD_TESTS`: Build tests (default: OFF)

Example:

```bash
cmake -DBUILD_DEMOS=ON -DBUILD_TESTS=ON ..
make
```

### Use Individual Libraries

Each library can be integrated into your CMake project using `add_subdirectory()`:

```cmake
add_subdirectory(path/to/act_cpp_packages/lib/act_logger)
target_link_libraries(your_target PRIVATE act_logger)
```

## Development

### Development Container

The [dev container](.devcontainer/) runs the toolchain image the CI publishes,
`ghcr.io/all-circuits-technologies/act-cpp-packages:dev`, built from
[`.devcontainer/image/Dockerfile`](.devcontainer/image/Dockerfile): the C++ toolchain, LLVM 21
(clang-format, clang-tidy, clangd), cpp-linter and the libraries the packages link against. Nothing
is built on your machine, and a check that passes in the container passes in the CI. The image is
refreshed before every start; offline, the container starts on the image you already have.

### Continuous Integration

Every push to `master` and every pull request runs [`ci.yml`](.github/workflows/ci.yml): it builds
and publishes the toolchain image when its Dockerfile changed, then formats and analyzes every
source inside it with cpp-linter. The findings are posted on the pull request.

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
