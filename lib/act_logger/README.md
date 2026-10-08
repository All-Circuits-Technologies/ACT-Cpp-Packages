<!-- SPDX-FileCopyrightText: 2026 Théo Magne <theo.magne@allcircuits.com> -->

<!-- SPDX-License-Identifier: LicenseRef-ALLCircuits-ACT-1.1 -->

# act_logger

Logging infrastructure library providing logger managers, external loggers, console output, and
helper utilities for consistent logging across modules. It implements the logger interface of
act_foundation (`AbsLogger`, `LogsLevel`, `LoggerStream`).

## Dependencies

- act_foundation
- act_text

## Features

- **Constants:** Logging level constants
- **Helpers:** Logger helper, multi-external logger, sub-logger helper
- **Printers:** Abstract external logger interface, standard console logger
- **Services:** Abstract logger manager, logger manager implementation
