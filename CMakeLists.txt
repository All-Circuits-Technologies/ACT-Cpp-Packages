# SPDX-FileCopyrightText: 2026 Théo Magne <theo.magne@allcircuits.com>
#
# SPDX-License-Identifier: LicenseRef-ALLCircuits-ACT-1.1

# =============================================================================
# ACT C++ Packages - Root CMakeLists.txt
# =============================================================================
#
# Modular C++ library collection:
#
#   - db_core    : Core database management classes
#   - foundation : Base classes and design patterns (no dependencies)
#   - logger     : Logging infrastructure and services
#   - threading  : Threading utilities and synchronization
#   - text       : String and text processing utilities
#   - files      : File system operations and utilities
#   - time       : Date, time, and timing utilities
#   - system     : System-level utilities (critical sections, system management)
#   - sqlite     : SQLite database management layer
#   - linux_io   : Linux-specific I/O (GPIO, LEDs)
#   - property   : Engine-agnostic persistent configuration store
#   - property_sqlite : SQLite backed property store
#
# =============================================================================

cmake_minimum_required(VERSION 3.23)

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

project(act_cpp_packages
    VERSION 1.0.0
    DESCRIPTION "ACT C++ modular library packages"
    LANGUAGES CXX
)

# -----------------------------------------------------------------------------
# Build Options (DOCUMENTATION ONLY)
# -----------------------------------------------------------------------------

option(BUILD_DEMOS "Build demo applications" OFF)
option(BUILD_TESTS "Build tests" OFF)

# -----------------------------------------------------------------------------
# Library Modules
#-----------------------------------------------------------------------------
# Modules are listed alphabetically for simplicity.
# -----------------------------------------------------------------------------

add_subdirectory(lib/act_db_core)
add_subdirectory(lib/act_files)
add_subdirectory(lib/act_foundation)
add_subdirectory(lib/act_linux_io)
add_subdirectory(lib/act_logger)
add_subdirectory(lib/act_property_core)
add_subdirectory(lib/act_property_sqlite)
add_subdirectory(lib/act_db_sqlite)
add_subdirectory(lib/act_system)
add_subdirectory(lib/act_text)
add_subdirectory(lib/act_threading)
add_subdirectory(lib/act_time)

# -----------------------------------------------------------------------------
# Demo Applications (TODO)
# -----------------------------------------------------------------------------
# apps/
#   <demo_name>/
#     CMakeLists.txt
#     main.cpp

if(BUILD_DEMOS)
    message(STATUS "Demo applications not yet implemented")
    # add_subdirectory(apps)
endif()

# -----------------------------------------------------------------------------
# Tests (TODO)
# -----------------------------------------------------------------------------
# tests/
#   <module>_tests/
#     CMakeLists.txt
#     test_*.cpp

if(BUILD_TESTS)
    message(STATUS "Tests not yet implemented")
    # enable_testing()
    # add_subdirectory(tests)
endif()
