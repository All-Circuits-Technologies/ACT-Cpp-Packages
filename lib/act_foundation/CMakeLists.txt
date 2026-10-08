# SPDX-FileCopyrightText: 2026 Théo Magne <theo.magne@allcircuits.com>
#
# SPDX-License-Identifier: LicenseRef-ALLCircuits-ACT-1.1

# -----------------------------------------------------------------------------
# act_foundation - Base classes and patterns
# -----------------------------------------------------------------------------
# Foundational types and design patterns used across all modules.
# Provides base classes, constants, common patterns (singleton, RAII) and the logger interface
# the other modules log through.
#
# This module has no internal dependencies.
# -----------------------------------------------------------------------------

add_library(act_foundation STATIC
    # Logger interface
    src/logger/logger_stream.cpp

    # Services
    src/services/abs_manager.cpp
    src/services/abs_service.cpp
    src/services/abs_with_life_cycle.cpp
)

# Public headers are exposed via include/<module_name>/
target_include_directories(act_foundation
    PUBLIC
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
    $<INSTALL_INTERFACE:include>
)

# C++20 required
target_compile_features(act_foundation PUBLIC cxx_std_20)

# Install targets
install(TARGETS act_foundation DESTINATION lib)
install(
    DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/include/"
    DESTINATION "include"
    FILES_MATCHING PATTERN "*.hpp"
)
