<!-- SPDX-FileCopyrightText: 2026 Théo Magne <theo.magne@allcircuits.com> -->

<!-- SPDX-License-Identifier: LicenseRef-ALLCircuits-ACT-1.1 -->

# act_foundation

Foundational types and design patterns used across all ACT C++ modules.

## Description

This module provides core base classes, constants, and common design patterns that are used
throughout the ACT C++ package ecosystem. It has no dependencies on other ACT modules and serves as
the foundation layer.

## Contents

- **Base Classes:** NotCopiableNotMovable, AbsManager, AbsService, AbsWithLifeCycle
- **Constants:** Software-wide constants and macros (def_soft.hpp)
- **Logger interface:** AbsLogger, the interface every module logs through, with its levels
  (LogsLevel) and its stream syntax (LoggerStream); the implementations are provided elsewhere
- **Patterns:** Singleton pattern, Finally (RAII guard)

## Dependencies

None - this is a foundational module.

## Usage

```cpp
#include "act_foundation/constants/def_soft.hpp"
#include "act_foundation/logger/abs_logger.hpp"
#include "act_foundation/services/abs_manager.hpp"
#include "act_foundation/services/abs_service.hpp"
#include "act_foundation/services/abs_with_life_cycle.hpp"
#include "act_foundation/not_copiable_not_movable.hpp"
```

## License

LicenseRef-ALLCircuits-ACT-1.1
